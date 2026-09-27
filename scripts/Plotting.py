
import copy
import pandas as pd
import numpy as np
from EnvironmentBuilder import MDPFactory
import matplotlib.pyplot as plt
from scripts.TexTables import SaveDataFrameToTexTemplate
from matplotlib.ticker import MaxNLocator
import math
from collections.abc import Mapping
from numbers import Real
from statistics import NormalDist
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from matplotlib.legend_handler import HandlerTuple
try:
    from scipy.stats import t as student_t
except ImportError:  # SciPy is optional in the notebook environment.
    student_t = None



def MultiPlot(inp_df, inp_configs, ind_var, ind_var_title,
              dep_var=None, dep_var_title=None, tile_var=None, tile_range=None, tile_dep_vars=None,
              group_var=None, group_list=None, group_labels=None,
              legend_loc="lower center", legend_cols=1, bbox_to_anchor=(0.5, 0.01), ind_range=None,
              constrained_layout=True, title="Average moral policies against unknown depth",
              file_out=None, styles=None, tile_dep_var_titles=None, sharey=None, log_scale=False,
              regression=False, regression_label=False, regression_values_in_legend=False
):
    """Plot configuration averages in a grid of subplots.

    There are two supported tiling modes:

    * ``dep_var`` + ``tile_var``: one subplot for each value of ``tile_var``.
    * ``tile_dep_vars``: one subplot for each named dependent-variable column.

    ``tile_dep_var_titles`` may be a sequence matching ``tile_dep_vars`` or a
    mapping from column name to display title. If omitted, column names are
    converted from snake case to display text.
    """
    styles = {} if styles is None else styles
    tile_dep_vars = None if tile_dep_vars is None else list(tile_dep_vars)
    if sharey is None:
        sharey = not bool(tile_dep_vars)

    if tile_dep_vars and (dep_var is not None or tile_var is not None):
        raise ValueError(
            "Use either tile_dep_vars, or dep_var together with tile_var; do not mix both modes."
        )
    # Require either a dependent variable column, or tile_dep_vars.
    if not tile_dep_vars and dep_var is None:
        raise ValueError(
            "Provide tile_dep_vars, or provide dep_var to plot as a single tile or tiled by tile_var."
        )

    group_key = group_var or "Configuration"

    required_columns = {group_key, ind_var}
    # If tiling by dependent variables, require those columns; otherwise require dep_var.
    if tile_dep_vars:
        required_columns.update(tile_dep_vars)
    else:
        required_columns.add(dep_var)
        if tile_var is not None:
            required_columns.add(tile_var)
    missing_columns = sorted(required_columns.difference(inp_df.columns))
    if missing_columns:
        raise KeyError(f"Data frame is missing columns: {', '.join(missing_columns)}")

    inp_df = inp_df.copy()
    # Filter either by explicit group_list (if grouping) or by inp_configs (legacy behaviour).
    if group_var is not None:
        if group_list is None:
            group_list = list(pd.unique(inp_df[group_var]))
        else:
            group_list = list(group_list)
        if group_labels is None:
            group_label_dict = {g: g for g in group_list}
        elif isinstance(group_labels, dict):
            group_label_dict = {g: group_labels.get(g, g) for g in group_list}
        else:
            group_labels = list(group_labels)
            if len(group_labels) != len(group_list):
                raise ValueError("group_labels must have one label per group_list entry.")
            group_label_dict = dict(zip(group_list, group_labels))
        inp_df = inp_df[inp_df[group_var].isin(group_list)].copy()
    else:
        inp_df = inp_df[inp_df["Configuration"].isin(inp_configs)].copy()
    if ind_range is not None:
        inp_df = inp_df[inp_df[ind_var].isin(ind_range)]

    if tile_dep_vars:
        avg_df = (
            inp_df
            .groupby([group_key, ind_var], as_index=False)[tile_dep_vars]
            .mean()
            .sort_values([group_key, ind_var])
        )

        if tile_dep_var_titles is None:
            display_titles = [column.replace("_", " ") for column in tile_dep_vars]
        elif isinstance(tile_dep_var_titles, dict):
            display_titles = [tile_dep_var_titles.get(column, column) for column in tile_dep_vars]
        else:
            display_titles = list(tile_dep_var_titles)
            if len(display_titles) != len(tile_dep_vars):
                raise ValueError("tile_dep_var_titles must have one title per tile_dep_vars column.")

        tiles = [
            (display_title, avg_df, column)
            for column, display_title in zip(tile_dep_vars, display_titles)
        ]
    else:
        # If tile_var is not provided, create a single tile with the whole data.
        if tile_var is None:
            avg_df = (
                inp_df
                .groupby([group_key, ind_var], as_index=False)[dep_var]
                .mean()
                .sort_values([group_key, ind_var])
            )
            tiles = [
                (dep_var_title or dep_var, avg_df, dep_var)
            ]
        else:
            if tile_range is None:
                tile_range = list(inp_df[tile_var].drop_duplicates())

            avg_df = (
                inp_df
                .groupby([tile_var, group_key, ind_var], as_index=False)[dep_var]
                .mean()
                .sort_values([tile_var, group_key, ind_var])
            )
            tiles = [
                (f"{tile_var} {tile_value}", avg_df[avg_df[tile_var] == tile_value], dep_var)
                for tile_value in tile_range
            ]

    if not tiles:
        raise ValueError("No subplots can be created from the supplied data and tile selection.")

    # Set the number of rows and columns based on the number of tiles.
    if len(tiles) <= 3:
        nrows = 1
        ncols = len(tiles)
    elif len(tiles) == 4:
        nrows = 2
        ncols = 2
    else:
        ncols = 3
        nrows = math.ceil(len(tiles) / ncols)

    fig, axes = plt.subplots(
        nrows=nrows,
        ncols=ncols,
        figsize=(6.5 * ncols, 5 * nrows),
        sharex=True,
        sharey=sharey,
        constrained_layout=constrained_layout,
    )
    if not constrained_layout:
        fig.subplots_adjust(
            left=0.08, right=0.97, bottom=0.19, top=0.90,
            hspace=0.2, wspace=0.05,
        )

    axes = np.atleast_1d(axes).flatten()

    for ax, (tile_title, curr_tile_df, value_column) in zip(axes, tiles):
        # Decide iteration order: either explicit group_list (when grouping) or inp_configs.
        if group_var is not None:
            iterate_over = group_list
        else:
            iterate_over = inp_configs

        for group in iterate_over:
            group_df = curr_tile_df[
                curr_tile_df[group_key] == group
            ].sort_values(ind_var)

            if group_df.empty:
                continue

            ax.plot(
                group_df[ind_var],
                group_df[value_column],
                markersize=8,
                label=(group_label_dict[group] if group_var is not None else group),
                **styles.get(group, {}),
            )
            observed_line_color = None
        if log_scale:
            ax.set_yscale("log")
        ax.set_title(tile_title)
        ax.set_xlabel(ind_var_title)
        ax.grid(True, alpha=0.3)

        x_ticks = sorted(curr_tile_df[ind_var].dropna().unique())
        ax.set_xticks(x_ticks)
        ax.set_xticklabels([
            f"{x:g}" if isinstance(x, (int, float, np.number)) else str(x)
            for x in x_ticks
        ])
        # Add optional regression per plotted series
        if regression:
            for group in (group_list if group_var is not None else inp_configs):
                group_df = curr_tile_df[curr_tile_df[group_key] == group].sort_values(ind_var)
                if group_df.empty:
                    continue
                x = group_df[ind_var].to_numpy(dtype=float)
                y = group_df[value_column].to_numpy(dtype=float)
                valid = (np.isfinite(x) & np.isfinite(y) & (y > 0))
                x_valid = x[valid]
                y_valid = y[valid]
                if x_valid.size < 2:
                    continue
                log_y = np.log(y_valid)
                growth_rate, intercept = np.polyfit(x_valid, log_y, deg=1)
                scale = np.exp(intercept)
                regression_x = np.linspace(x_valid.min(), x_valid.max(), 200)
                regression_y = scale * np.exp(growth_rate * regression_x)
                label_text = (
                    (group_label_dict[group] if group_var is not None else group)
                    + f" fit: $y={scale:.3g}e^{{{growth_rate:.3g}x}}$"
                )
                ax.plot(
                    regression_x,
                    regression_y,
                    linestyle="--",
                    linewidth=2,
                    alpha=0.9,
                    label=(label_text if regression_values_in_legend else "__nolegend__"),
                )
                if regression_label:
                    # place small text box on the plot for this tile
                    ax.text(0.02, 0.98, label_text,
                            transform=ax.transAxes, va="top", ha="left",
                            fontsize=9, linespacing=1.2,
                            bbox={"boxstyle": "round,pad=0.3", "facecolor": "white", "edgecolor": "0.7", "alpha": 0.9})

    # Remove unused panels if the grid has more axes than tiles.
    for ax in axes[len(tiles):]:
        ax.remove()

    # Label only the left-hand plots to avoid repeating the common unit.
    for i, ax in enumerate(axes[:len(tiles)]):
        if i % ncols == 0:
            ax.set_ylabel(dep_var_title or "Value")

    # Construct the legend explicitly to preserve inp_configs order.
    handles_by_label = {}
    for ax in axes[:len(tiles)]:
        handles, labels = ax.get_legend_handles_labels()
        for handle, label in zip(handles, labels):
            handles_by_label[label] = handle

    legend_labels = [config for config in inp_configs if config in handles_by_label]
    legend_handles = [handles_by_label[config] for config in legend_labels]

    fig.legend(
        legend_handles,
        legend_labels,
        title="Configuration",
        loc=legend_loc,
        bbox_to_anchor=bbox_to_anchor,
        ncols=legend_cols,
        frameon=False,
    )
    fig.suptitle(title, fontsize=20)

    if file_out is not None:
        fig.savefig(file_out, bbox_inches="tight")
    plt.show()
    return fig, axes[:len(tiles)]


def QuickPlotGraph(df_, configs:list,
                   independent_var:str, 
                   dependent_var:str, 
                   config_labels:list=None,
                   title:str=None, 
                   ind_label:str=None, 
                   dep_label:str=None, 
                   file_out:str=None, 
                   regression:bool=False, 
                   regression_label:bool=False,
                   fig_size:tuple=(8,6),
                   log_scale=True,
                   colors:dict={},
                   regression_values_in_legend=False,
                   legend_title="Config",
                   legend_loc="upper left",
                   legend_bbox_to_anchor=None,
                   ):
    if ind_label is None:
        ind_label = independent_var
    if dep_label is None:
        dep_label = dependent_var
    if config_labels is None:
        config_labels = configs
    config_label_dict = {}
    for i in range(len(configs)):
        config_label_dict[configs[i]] = config_labels[i]
    
    df_a = df_.copy()
    df_a = df_a[df_a['Configuration'].isin(configs)]
    df_aa = (
        df_a
        .groupby(["Configuration", independent_var], as_index=False)[dependent_var]
        .mean()
        .sort_values(["Configuration", independent_var])
    )

    regression_results = {}
    regression_text = []

    fig, ax = plt.subplots(figsize=fig_size)
    for c, g in df_aa.groupby('Configuration', sort=False):
        x = g[independent_var].to_numpy(dtype=float)
        y = g[dependent_var].to_numpy(dtype=float)
        ax.scatter(x, y, alpha=0.75, label=config_label_dict[c])
        observed_line, = ax.plot(x, y, alpha=0.75, color = colors[c] if c in colors.keys() else None)

        if regression:
            # log(y) is only defined for strictly positive y values.
            valid = (np.isfinite(x) & np.isfinite(y) & (y > 0))
            x_valid = x[valid]
            y_valid = y[valid]
            
            log_y = np.log(y_valid)

            # log(y) = intercept + growth_rate * x
            growth_rate, intercept = np.polyfit(x_valid, log_y, deg=1)

            scale = np.exp(intercept)

            predicted_log_y = (intercept + growth_rate * x_valid)

            residual_sum_of_squares = np.sum((log_y - predicted_log_y) ** 2)
            total_sum_of_squares = np.sum((log_y - log_y.mean()) ** 2)

            if np.isclose(total_sum_of_squares, 0):
                r_squared = np.nan
            else:
                r_squared = (1 - residual_sum_of_squares / total_sum_of_squares)

            regression_results[c] = { "a": scale, "b": growth_rate, "R_squared": r_squared}

            # Use a dense x range to draw a smooth exponential curve.
            regression_x = np.linspace(x_valid.min(), x_valid.max(), 200,)
            regression_y = scale * np.exp(growth_rate * regression_x) 

            regression_description = (
                rf"{config_label_dict[c]} fit: "
                rf"$y={scale:.3g}e^{{{growth_rate:.3g}x}}$, "
                rf"$R^2={r_squared:.4f}$"
            )
            ax.plot(
                regression_x,
                regression_y,
                linestyle="--",
                linewidth=2,
                alpha=0.9,
                color=observed_line.get_color(),
                label=(
                    regression_description
                    if regression_values_in_legend
                    else "__nolegend__"
                ),
            )

            regression_text.append(regression_description)
    if log_scale:
        plt.yscale('log')  # Set the y-axis to logarithmic scale
    if (title is None):
        plt.title(f"{ind_label} vs {dep_label}")
    else:
        plt.title(title)

    plt.xlabel(ind_label)
    plt.ylabel(dep_label)

    ax.grid(True, alpha=0.3)
    ax.legend(
        title=legend_title,
        loc=legend_loc,
        bbox_to_anchor=legend_bbox_to_anchor,
    )

    if regression and regression_text and regression_label:
        regression_summary = "\n".join(regression_text)
        # Position the regression summary outside the plotting region.
        ax.text(1.03, 0.98, regression_summary,
            transform=ax.transAxes, va="top", ha="left",
            fontsize=11, linespacing=1.4,
            bbox={"boxstyle": "round,pad=0.5", "facecolor": "white", "edgecolor": "0.7", "alpha": 0.9,},
        )
        # Reserve space on the right for the summary.
        fig.subplots_adjust(right=0.68)
    else:
        fig.tight_layout()

    plt.tight_layout() 
    if (not file_out is None):
        plt.savefig(f"{file_out}")
    plt.show()  # Show the plot



def MultiLinePlotGraph(df_,
                    independent_var:str, 
                    dependent_var:str, 
                    group_var:str=None,
                    group_list:list=None,
                    group_labels:list=None,
                    fit_group_labels=None,
                    title:str=None, 
                    ind_label:str=None, 
                    dep_label:str=None, 
                    file_out:str=None, 
                    regression:bool=False, 
                    regression_label:bool=False,
                    fig_size:tuple=(8,6),
                    log_scale=True,
                    colors:dict=None,
                    styles:dict=None,
                    cmap:str='tab10',
                    xtick_step:float=None,
                    fit_legend_mode="legacy",       # "legacy", "inline", or "separate"
                    fit_legend_title="Fits",
                    fit_legend_loc="upper left",
                    fit_legend_bbox_to_anchor=None,
                    fit_styles:dict=None,
                    scatter_all_points:bool=False,
                    regression_values_in_legend=False,
                    legend_title="Config",
                    legend_loc="lower center",
                    legend_cols:int=1,
                    bbox_to_anchor=(0.5, 0.01),
                    legend_bbox_to_anchor=None,
                    legend_in_axes:bool=False,
                    fit_legend_in_axes:bool=False,
                    no_legend:bool=False,
                    legend_only:bool=False,
                    plot_scatter_line:bool=True,
                    reg_line_alpha:float=1,
                    legend_marker_alpha=None,
                    marker_size:float=6.0,
                    offset=None, # Can be {'group_A': (offset_x,offset_y)}
                   ):
    if ind_label is None:
        ind_label = independent_var
    if dep_label is None:
        dep_label = dependent_var

    if group_var is None:
        # Single series (no grouping). Use a "None" key internally so
        # the rest of the code can treat a single series like a group.
        group_list = [None]

        # By default do not add a legend label for the single series
        # (previous behaviour used the label "Series"). If the user
        # explicitly provides `group_labels`, respect it.
        if group_labels is None:
            group_labels = [None]
        elif isinstance(group_labels, str):
            group_labels = [group_labels]
        elif not isinstance(group_labels, (list, tuple, pd.Index, np.ndarray)):
            group_labels = [group_labels]
        elif len(group_labels) != 1:
            group_labels = [group_labels[0]]

        if fit_group_labels is None:
            fit_group_labels = group_labels

        group_label_dict = {None: group_labels[0]}

        df_a = df_.copy()
        df_aa = (
            df_a[[independent_var, dependent_var]]
            .groupby(independent_var, as_index=False)[dependent_var]
            .mean()
            .sort_values(independent_var)
        )
    else:
        if group_list is None:
            group_list = list(pd.unique(df_[group_var]))
        if group_labels is None:
            group_labels = group_list
        elif isinstance(group_labels, dict):
            group_labels = [group_labels.get(g, g) for g in group_list]
        elif not isinstance(group_labels, (list, tuple, pd.Index, np.ndarray)):
            group_labels = [group_labels]
        if fit_group_labels is None:
            fit_group_labels = group_labels
        if len(group_labels) != len(group_list):
            raise ValueError("group_labels must have one label per group_list element.")
        if len(fit_group_labels) != len(group_list):
            raise ValueError("fit_group_labels must have one label per group_list element.")

        group_label_dict = {g: group_labels[i] for i, g in enumerate(group_list)}
        df_a = df_.copy()
        df_a = df_a[df_a[group_var].isin(group_list)]
        df_aa = (
            df_a
            .groupby([group_var, independent_var], as_index=False)[dependent_var]
            .mean()
            .sort_values([group_var, independent_var])
        )

    regression_results = {}
    regression_text = []
    regression_equations = {}
    regression_handles = {}
    if colors is None:
        colors = {}
    if styles is None:
        styles = {}
    if fit_styles is None:
        fit_styles = {}

    def _as_xy_offset(value, description):
        if isinstance(value, Real):
            xy_offset = (0.0, float(value))
        elif (
            isinstance(value, (tuple, list, np.ndarray))
            and len(value) == 2
            and all(isinstance(component, Real) for component in value)
        ):
            xy_offset = tuple(float(component) for component in value)
        else:
            raise TypeError(
                f"{description} must be a number or a two-item (x, y) sequence."
            )

        if not all(np.isfinite(component) for component in xy_offset):
            raise ValueError(f"{description} must contain only finite values.")
        return xy_offset

    series_items = [None] if group_var is None else group_list
    if offset is None:
        series_offsets = {key: (0.0, 0.0) for key in series_items}
    elif isinstance(offset, Mapping):
        unknown_groups = set(offset).difference(series_items)
        if unknown_groups:
            raise ValueError(
                "offset contains groups that are not being plotted: "
                + ", ".join(map(str, unknown_groups))
            )
        series_offsets = {
            key: _as_xy_offset(offset.get(key, 0), f"offset[{key!r}]")
            for key in series_items
        }
    else:
        x_step, y_step = _as_xy_offset(offset, "offset")
        series_offsets = {
            key: (index * x_step, index * y_step)
            for index, key in enumerate(series_items)
        }

    # Make each nonzero translation explicit in the legend.
    for key, (x_offset, y_offset) in series_offsets.items():
        label = group_label_dict.get(key)
        if label is not None and (x_offset != 0 or y_offset != 0):
            offset_parts = []
            if x_offset != 0:
                offset_parts.append(f"x {x_offset:+g}")
            if y_offset != 0:
                offset_parts.append(f"y {y_offset:+g}")
            group_label_dict[key] = f"{label} (offset: {', '.join(offset_parts)})"

    # If there is only a single series (group_var is None) allow the
    # caller to pass a style dict directly (e.g. `styles={"color":"r"}`)
    # instead of a mapping from group keys to style dicts. Detect that
    # case and pull out the attributes into `single_series_style` while
    # leaving `styles` as a mapping for backward compatibility.
    single_series_style = None
    def _styles_looks_like_attr_dict(d: dict) -> bool:
        # Consider it an attr-dict if any value is not a dict (common
        # case: {"color": "r", "alpha": 0.8}).
        if not d:
            return False
        return any(not isinstance(v, dict) for v in d.values())

    if group_var is None and isinstance(styles, dict) and _styles_looks_like_attr_dict(styles):
        single_series_style = styles
        styles = {}
    # fit_styles can also be a single-attr dict for the lone-series case
    fit_single_style = None
    if group_var is None and isinstance(fit_styles, dict) and _styles_looks_like_attr_dict(fit_styles):
        fit_single_style = fit_styles
        fit_styles = {}

    fig, ax = plt.subplots(figsize=fig_size)
    if not colors:
        try:
            cmap_obj = plt.get_cmap(cmap)
            n = len(group_list)
            for i, g_key in enumerate(group_list):
                frac = i / (n - 1) if n > 1 else 0.5
                colors[g_key] = cmap_obj(frac)
        except Exception:
            for i, g_key in enumerate(group_list):
                colors[g_key] = None

    # If the caller only wants the legend, build proxy handles from the
    # configured styles and return a figure that contains only the legend.
    if legend_only:
        # Determine series keys and labels in the same order used elsewhere.
        series_keys = group_list if group_var is not None else [None]
        if group_var is None:
            series_labels = [group_label_dict.get(None)]
        else:
            series_labels = [group_label_dict[g] for g in group_list]

        series_handles = []
        kept_series_labels = []
        for key, label in zip(series_keys, series_labels):
            # Merge per-group styles and single-series style if present.
            try:
                group_style = {}
                if isinstance(styles, dict):
                    group_style.update(styles.get(key, {}) or {})
            except Exception:
                group_style = {}
            if single_series_style:
                try:
                    for k, v in single_series_style.items():
                        group_style.setdefault(k, v)
                except Exception:
                    pass

            color_for_legend = group_style.get("color")
            if color_for_legend is None:
                color_for_legend = colors.get(key) if (key in colors) else None
            if color_for_legend is None:
                color_for_legend = "black"

            marker_for_legend = group_style.get("marker")
            if marker_for_legend is None:
                marker_for_legend = "o"

            # Determine linestyle: prefer fit_styles, then group_style.
            linestyle_for_legend = None
            try:
                linestyle_for_legend = fit_styles.get(key, {}).get("linestyle")
            except Exception:
                linestyle_for_legend = None
            if linestyle_for_legend is None:
                linestyle_for_legend = group_style.get("linestyle")

            handle_linestyle = linestyle_for_legend if (linestyle_for_legend is not None) else ("-" if plot_scatter_line else "None")

            legend_linewidth = group_style.get("linewidth", 1.8)
            legend_markersize = group_style.get("markersize",
                                               group_style.get("ms",
                                                               (marker_size if marker_size is not None else 6)))

            handle_to_use = Line2D([0], [0], color=color_for_legend or "black",
                                   marker=marker_for_legend,
                                   linestyle=handle_linestyle,
                                   linewidth=legend_linewidth,
                                   markersize=legend_markersize)

            if legend_marker_alpha is not None:
                try:
                    handle_to_use.set_alpha(legend_marker_alpha)
                except Exception:
                    pass

            series_handles.append(handle_to_use)
            kept_series_labels.append(label)

        # Create a legend-only figure. Hide axes and show only legend.
        ax.axis('off')
        target = ax if legend_in_axes else fig
        target.legend(series_handles, kept_series_labels, title=legend_title,
                      loc=legend_loc,
                      bbox_to_anchor=(legend_bbox_to_anchor if legend_bbox_to_anchor is not None else bbox_to_anchor),
                      ncols=legend_cols, frameon=False)
        if file_out is not None:
            plt.savefig(f"{file_out}", bbox_inches='tight', pad_inches=0.1)
        plt.show()
        return fig, ax

    plotted_x_values = []
    for c in series_items:
        if group_var is None:
            x = df_aa[independent_var].to_numpy(dtype=float)
            y = df_aa[dependent_var].to_numpy(dtype=float)
            series_label = group_label_dict.get(None)
            grouped_values = df_aa
            c_key = None
        else:
            grouped_values = df_aa[df_aa[group_var] == c].sort_values(independent_var)
            x = grouped_values[independent_var].to_numpy(dtype=float)
            y = grouped_values[dependent_var].to_numpy(dtype=float)
            series_label = group_label_dict[c]
            c_key = c

        x_offset, y_offset = series_offsets[c_key]
        plotted_x = x + x_offset
        plotted_y = y + y_offset
        plotted_x_values.append(plotted_x)

        # Decide whether to scatter grouped means or all raw points.
        if scatter_all_points:
            if group_var is None:
                raw_df = df_a[[independent_var, dependent_var]].dropna()
            else:
                raw_df = df_a[df_a[group_var] == c][[independent_var, dependent_var]].dropna()
            scatter_x = raw_df[independent_var].to_numpy(dtype=float)
            scatter_y = raw_df[dependent_var].to_numpy(dtype=float)
        else:
            scatter_x = x
            scatter_y = y

        scatter_x = scatter_x + x_offset
        scatter_y = scatter_y + y_offset

        # Default marker size in points (for Line2D) and scatter `s` (points^2).
        default_marker_pts = float(marker_size)
        scatter_params = dict(s=(default_marker_pts ** 2), alpha=0.85, zorder=3)
        # Merge per-group styles and single-series style (if provided).
        group_style_for_scatter = styles.get(c_key, {}) if isinstance(styles, dict) else {}
        scatter_params.update(group_style_for_scatter)
        if single_series_style:
            scatter_params.update(single_series_style)
        # If markersize provided in styles (points), convert to scatter `s`.
        ms = None
        if isinstance(group_style_for_scatter, dict):
            ms = group_style_for_scatter.get("markersize") or group_style_for_scatter.get("ms")
        if ms is None and single_series_style:
            ms = single_series_style.get("markersize") or single_series_style.get("ms")
        if ms is not None:
            try:
                scatter_params["s"] = float(ms) ** 2
            except Exception:
                pass
        if c_key in colors and colors[c_key] is not None:
            scatter_params.setdefault("color", colors[c_key])
        # Ensure markers render above lines by using higher zorder for scatter.
        ax.scatter(scatter_x, scatter_y, label=series_label, **{k: v for k, v in scatter_params.items() if v is not None})

        if (plot_scatter_line):
            # Draw lines below markers so markers appear on top.
            line_params = dict(alpha=0.9, zorder=2)
            # Merge per-group styles and single-series style (if provided).
            line_params.update(styles.get(c_key, {}))
            if single_series_style:
                line_params.update(single_series_style)
            if c_key in colors and colors[c_key] is not None:
                line_params.setdefault("color", colors[c_key])
            ax.plot(
                plotted_x,
                plotted_y,
                **{k: v for k, v in line_params.items() if v is not None},
            )

        if regression:
            valid = (np.isfinite(x) & np.isfinite(y) & (y > 0))
            x_valid = x[valid]
            y_valid = y[valid]
            if x_valid.size >= 2:
                log_y = np.log(y_valid)
                growth_rate, intercept = np.polyfit(x_valid, log_y, deg=1)
                scale = np.exp(intercept)
                predicted_log_y = (intercept + growth_rate * x_valid)
                residual_sum_of_squares = np.sum((log_y - predicted_log_y) ** 2)
                total_sum_of_squares = np.sum((log_y - log_y.mean()) ** 2)
                r_squared = (
                    np.nan if np.isclose(total_sum_of_squares, 0)
                    else 1 - residual_sum_of_squares / total_sum_of_squares
                )
                regression_results[c_key] = {"a": scale, "b": growth_rate, "R_squared": r_squared}
                regression_x = np.linspace(x_valid.min(), x_valid.max(), 200)
                regression_y = scale * np.exp(growth_rate * regression_x)
                fit_equation = (
                    rf"$y={scale:.3g}e^{{{growth_rate:.3g}x}}$, "
                    rf"$R^2={r_squared:.4f}$"
                )
                regression_equations[c_key] = fit_equation
                regression_description = f"{series_label} fit: {fit_equation}"

                regression_description = (
                    rf"{series_label} fit: "
                    rf"$y={scale:.3g}e^{{{growth_rate:.3g}x}}$, "
                    rf"$R^2={r_squared:.4f}$"
                )

                if False:
                    # Build fit-line style: prefer `fit_styles`, then fall back to
                    # `styles`/`single_series_style`, then `colors`.
                    fit_line_params = dict(alpha=0.6, linestyle="--", linewidth=2)
                    fit_line_params.update(fit_styles.get(c_key, {}))
                    if fit_single_style:
                        fit_line_params.update(fit_single_style)
                    # Allow per-series styles to influence fits if not provided
                    # explicitly.
                    fit_line_params.update({k: v for k, v in styles.get(c_key, {}).items() if k not in fit_line_params})
                    if c_key in colors and colors[c_key] is not None:
                        fit_line_params.setdefault("color", colors[c_key])

                    fit_label = (regression_description if regression_values_in_legend else "__nolegend__")
                    fit_line_params["label"] = fit_label
                    # Remove None values
                    fit_line_params = {k: v for k, v in fit_line_params.items() if v is not None}
                    reg_line, = ax.plot(regression_x, regression_y, **fit_line_params)
                    #END
                
                # Merge per-group and single-series styles for the fit line
                line_params = dict(alpha=0.9)
                line_params.update(styles.get(c_key, {}))
                if single_series_style:
                    line_params.update(single_series_style)
                if c_key in colors and colors[c_key] is not None:
                    line_params.setdefault("color", colors[c_key])
                color_to_use = line_params.get("color", None)

                # Determine linestyle precedence: prefer explicit fit_styles, then
                # user-provided group `styles` (per request), then fall back to dashed.
                fit_linestyle = None
                # Check explicit fit_styles first
                try:
                    fit_linestyle = fit_styles.get(c_key, {}).get("linestyle")
                except Exception:
                    fit_linestyle = None
                # Then check styles for a provided linestyle when plot_scatter_line is False
                if fit_linestyle is None and (not plot_scatter_line):
                    try:
                        fit_linestyle = styles.get(c_key, {}).get("linestyle")
                    except Exception:
                        fit_linestyle = None

                used_linestyle = fit_linestyle if (fit_linestyle is not None) else "--"

                reg_line, = ax.plot(
                    regression_x + x_offset,
                    regression_y + y_offset,
                    linestyle=used_linestyle,
                    linewidth=2,
                    color=color_to_use,
                    label=(regression_description if regression_values_in_legend else "__nolegend__"),
                    alpha=reg_line_alpha,
                    zorder=2,
                )
                #END AGAIN

                if regression_values_in_legend:
                    regression_handles[c_key] = reg_line
                regression_text.append(regression_description)

    if log_scale:
        plt.yscale('log')
    if title is not None:
        plt.title(title)

    plt.xlabel(ind_label)
    plt.ylabel(dep_label)
    ax.grid(True, alpha=0.3)

    if xtick_step is not None:
        all_x = (
            np.concatenate(plotted_x_values)
            if plotted_x_values
            else np.array([], dtype=float)
        )
        all_x = all_x[np.isfinite(all_x)]
        if all_x.size:
            xmin, xmax = float(np.min(all_x)), float(np.max(all_x))
            xticks = np.arange(xmin, xmax + xtick_step * 0.5, xtick_step)
            ax.set_xticks(xticks)

    def _copy_handle_with_alpha(handle, alpha):
        if alpha is None:
            return handle
        try:
            handle_copy = copy.copy(handle)
            handle_copy.set_alpha(alpha)
            return handle_copy
        except Exception:
            return handle

    if not no_legend:
        handles, labels = ax.get_legend_handles_labels()

        from collections import OrderedDict
        label_to_handle = OrderedDict()
        for handle, label in zip(handles, labels):
            if label == "__nolegend__":
                continue
            if label not in label_to_handle:
                label_to_handle[label] = handle

        # Original series, in the requested group order.
        if group_var is not None:
            series_keys = group_list
            series_labels = [group_label_dict[g] for g in group_list]
        else:
            series_keys = [None]
            series_labels = [group_label_dict[None]]

        # Build series handles in order from merged style properties so that
        # legend markers, linestyles and colours match what was plotted.
        series_handles = []
        kept_series_labels = []
        for key, label in zip(series_keys, series_labels):
            if label not in label_to_handle:
                continue
            base_handle = label_to_handle[label]

            # Merge per-group styles and single-series style if present.
            try:
                group_style = {}
                if isinstance(styles, dict):
                    group_style.update(styles.get(key, {}) or {})
            except Exception:
                group_style = {}
            if single_series_style:
                try:
                    # single_series_style provides defaults; do not overwrite
                    # explicit per-group attributes with None values.
                    for k, v in single_series_style.items():
                        group_style.setdefault(k, v)
                except Exception:
                    pass

            # Determine color: prefer explicit group style, then colors dict,
            # then fall back to extracting from the original handle.
            color_for_legend = group_style.get("color")
            if color_for_legend is None:
                color_for_legend = colors.get(key) if (key in colors) else None
            if color_for_legend is None:
                try:
                    if hasattr(base_handle, "get_color"):
                        color_for_legend = base_handle.get_color()
                    elif hasattr(base_handle, "get_facecolor"):
                        fc = base_handle.get_facecolor()
                        color_for_legend = fc[0] if len(fc) else None
                except Exception:
                    color_for_legend = None

            # Determine marker: prefer group style, else try to infer from base handle
            marker_for_legend = group_style.get("marker")
            if marker_for_legend is None:
                try:
                    if hasattr(base_handle, "get_marker"):
                        marker_for_legend = base_handle.get_marker()
                except Exception:
                    marker_for_legend = None
            if marker_for_legend is None:
                marker_for_legend = "o"

            # Determine linestyle: prefer fit_styles, then group_style; if
            # absent and plot_scatter_line is True we can show a solid line,
            # otherwise show marker-only in the legend.
            linestyle_for_legend = None
            try:
                linestyle_for_legend = fit_styles.get(key, {}).get("linestyle")
            except Exception:
                linestyle_for_legend = None
            if linestyle_for_legend is None:
                linestyle_for_legend = group_style.get("linestyle")

            # Build Line2D legend handle using determined attributes. If no
            # linestyle is available, set linestyle='None' to display marker only.
            handle_linestyle = linestyle_for_legend if (linestyle_for_legend is not None) else ("-" if plot_scatter_line else "None")

            legend_linewidth = group_style.get("linewidth", 1.8)
            # Prefer explicit group markersize, then single-style ms, then
            # function-level marker_size parameter, then default 6.
            legend_markersize = group_style.get("markersize",
                                               group_style.get("ms",
                                                               (marker_size if marker_size is not None else 6)))

            handle_to_use = Line2D([0], [0], color=color_for_legend or "black",
                                   marker=marker_for_legend,
                                   linestyle=handle_linestyle,
                                   linewidth=legend_linewidth,
                                   markersize=legend_markersize)

            # Apply requested legend marker alpha if provided
            if legend_marker_alpha is not None:
                try:
                    handle_to_use.set_alpha(legend_marker_alpha)
                except Exception:
                    pass

            series_handles.append(handle_to_use)
            kept_series_labels.append(label)

        # Replace series_labels with only those kept (have handles)
        series_labels = kept_series_labels

        if regression_values_in_legend and fit_legend_mode == "inline":
            # One row per configuration:
            # marker/line -> configuration -> corresponding fit equation.
            inline_labels = []

            # Special handling for the single-series (no group) case: if
            # the caller requested fit values in the legend, prefer the
            # captured regression handle so the fit shows up even when no
            # series legend exists.
            if group_var is None:
                if regression_values_in_legend and (None in regression_handles):
                    eq = regression_equations.get(None, "")
                    lbl = group_label_dict.get(None) or ""
                    label_text = f"{lbl}\n{eq}" if lbl else f"{eq}"
                    target = ax if legend_in_axes else fig
                    target.legend([regression_handles[None]], [label_text], title=legend_title,
                                  loc=legend_loc,
                                  bbox_to_anchor=(legend_bbox_to_anchor if legend_bbox_to_anchor is not None else bbox_to_anchor),
                                  ncols=legend_cols, frameon=False)
                else:
                    # nothing to show
                    pass
            else:
                for key, label in zip(series_keys, series_labels):
                    equation = regression_equations.get(key)
                    lbl = label if label is not None else ""

                    if equation is not None:
                        inline_labels.append(f"{lbl}\n{equation}")
                    else:
                        inline_labels.append(lbl)
                target = ax if legend_in_axes else fig
                target.legend(
                    series_handles,
                    inline_labels,
                    title=legend_title,
                    loc=legend_loc,
                    bbox_to_anchor=(
                        legend_bbox_to_anchor
                        if legend_bbox_to_anchor is not None
                        else bbox_to_anchor
                    ),
                    ncols=legend_cols,
                    frameon=False,
                )

        elif regression_values_in_legend and fit_legend_mode == "separate":
            # First legend: configurations only. Allow placing legends
            # inside the axes by selecting the target (ax) instead of fig.
            if legend_in_axes:
                config_legend = ax.legend(
                    series_handles,
                    series_labels,
                    title=legend_title,
                    loc=legend_loc,
                    bbox_to_anchor=(
                        legend_bbox_to_anchor
                        if legend_bbox_to_anchor is not None
                        else bbox_to_anchor
                    ),
                    ncols=legend_cols,
                    frameon=False,
                )
            else:
                config_legend = fig.legend(
                    series_handles,
                    series_labels,
                    title=legend_title,
                    loc=legend_loc,
                    bbox_to_anchor=(
                        legend_bbox_to_anchor
                        if legend_bbox_to_anchor is not None
                        else bbox_to_anchor
                    ),
                    ncols=legend_cols,
                    frameon=False,
                )

            # Keep the first legend when adding the second.
            # For axes legends, ensure both are visible by re-adding the
            # first as an artist after creating the second.
            fit_handles = []
            fit_labels = []

            for key, label in zip(series_keys, series_labels):
                equation = regression_equations.get(key)

                if equation is None:
                    continue

                fit_handles.append(
                    _copy_handle_with_alpha(
                        Line2D([0], [0],
                            color=colors.get(key, "black"),
                            linestyle="--",
                            linewidth=2,
                        ),
                        legend_marker_alpha,
                    )
                )

                lbl = label if label is not None else ""
                fit_labels.append(f"{lbl}: {equation}" if lbl else f"{equation}")

            if fit_handles:
                if fit_legend_in_axes:
                    fit_legend = ax.legend(
                        fit_handles,
                        fit_labels,
                        title=fit_legend_title,
                        loc=fit_legend_loc,
                        bbox_to_anchor=fit_legend_bbox_to_anchor,
                        frameon=False,
                    )
                    # Re-add the configuration legend so it stays visible.
                    if legend_in_axes and config_legend is not None:
                        ax.add_artist(config_legend)
                else:
                    fig.legend(
                        fit_handles,
                        fit_labels,
                        title=fit_legend_title,
                        loc=fit_legend_loc,
                        bbox_to_anchor=fit_legend_bbox_to_anchor,
                        frameon=False,
                    )

        else:
            # LEGACY MODE -- preserves the old behaviour.
            ordered_labels = list(series_labels)
            ordered_handles = list(series_handles)

            if regression_values_in_legend:
                extra_labels = [
                    label for label in label_to_handle
                    if label not in ordered_labels
                ]

                ordered_labels.extend(extra_labels)
                ordered_handles.extend(
                    label_to_handle[label]
                    for label in extra_labels
                )

            if ordered_handles:
                fig.legend(
                    ordered_handles,
                    ordered_labels,
                    title=legend_title,
                    loc=legend_loc,
                    bbox_to_anchor=(
                        legend_bbox_to_anchor
                        if legend_bbox_to_anchor is not None
                        else bbox_to_anchor
                    ),
                    ncols=legend_cols,
                    frameon=False,
                )


    if regression and regression_text and regression_label:
        regression_summary = "\n".join(regression_text)
        ax.text(1.03, 0.98, regression_summary,
            transform=ax.transAxes, va="top", ha="left",
            fontsize=11, linespacing=1.4,
            bbox={"boxstyle": "round,pad=0.5", "facecolor": "white", "edgecolor": "0.7", "alpha": 0.9},
        )
        fig.subplots_adjust(right=0.68)
    else:
        fig.tight_layout()

    if file_out is not None:
        plt.savefig(f"{file_out}", bbox_inches="tight", pad_inches=0.05)
    plt.show()
    return fig, ax

def SafeGetStyle(styles, category, style_name):
    x = None
    try:
        x = styles[category][style_name]
    except:
        x = None
    return x


def mean_time_graph(df, config_name, file_out=None, title=None, fig_size:tuple=(8,6), dep_var="Horizon", dep_var_label=None,
                    time_categories=["Heuristic Time", "Planning Time", "Solution Extraction Time", "MEHR Time"],
                    ticks_for_existing_values_only=True, 
                    legend_title="Time category", legend_cols=1, legend_loc="best", legend_bbox_to_anchor=None, 
                    ci=None, y_label=None, styles=None, plot_points=None, # None, 'mean' or 'all'
                    hatch_graph_repeat=1, hatch_legend_repeat=3, hatch_graph_linewidth=None, hatch_legend_linewidth=1.2,
                    legend_only:bool=False
                    ):

    df_a = df[df["Configuration"] == config_name].copy()

    # Percentage of total time taken by each category for each run
    percentage_columns = []

    for category in time_categories:
        percentage_column = f"{category} Percentage"
        df_a[percentage_column] = df_a[category].div(df_a["Total Time"]).mul(100)
        percentage_columns.append(percentage_column)

    cii = ci if not ci is None else 0.95
    summary = df_a.groupby(dep_var, as_index=False)[percentage_columns].mean().sort_values(dep_var)

    low_q = (1-cii) / 2
    high_q = 1 - low_q
    lo = df_a.groupby(dep_var, as_index=False)[percentage_columns].quantile(low_q).sort_values(dep_var).reset_index(drop=True)
    hi = df_a.groupby(dep_var, as_index=False)[percentage_columns].quantile(high_q).sort_values(dep_var).reset_index(drop=True)
    # Mean percentage at each horizon
    
    fig, ax = plt.subplots(figsize=fig_size)

    colors = plt.get_cmap("tab10").colors
    hatches = ['/', '\\', '|', '-', '+', 'x', 'o', 'O', '.', '*']
    markers = ['o', 'x', '^', '*', 'h']
    linestyles = ['-', '--', '-.', ':']
    styles_ = {}
    if styles is not None:
        styles_ = styles.copy()
    for i, (category, percentage_column) in enumerate(zip(time_categories, percentage_columns)):
        styles_.setdefault(category, {})
        color = SafeGetStyle(styles, category, 'color')
        if color is None:
            color = colors[i % len(colors)]
        styles_[category]['color'] = color
        hatch = SafeGetStyle(styles, category, 'hatch')
        if hatch is None:
            hatch = hatches[i % len(hatches)]
        styles_[category]['hatch'] = hatch
        # Per-category hatch repetition and linewidth to improve legend visibility
        hg_repeat = SafeGetStyle(styles, category, 'hatch_graph_repeat')
        if hg_repeat is None:
            hg_repeat = hatch_graph_repeat
        try:
            hg_repeat = int(hg_repeat)
        except Exception:
            hg_repeat = 1
        styles_[category]['hatch_graph_repeat'] = hg_repeat

        hl_repeat = SafeGetStyle(styles, category, 'hatch_legend_repeat')
        if hl_repeat is None:
            hl_repeat = hatch_legend_repeat
        try:
            hl_repeat = int(hl_repeat)
        except Exception:
            hl_repeat = hatch_legend_repeat
        styles_[category]['hatch_legend_repeat'] = hl_repeat

        hg_lw = SafeGetStyle(styles, category, 'hatch_graph_linewidth')
        if hg_lw is None:
            hg_lw = hatch_graph_linewidth
        styles_[category]['hatch_graph_linewidth'] = hg_lw

        hl_lw = SafeGetStyle(styles, category, 'hatch_legend_linewidth')
        if hl_lw is None:
            hl_lw = hatch_legend_linewidth
        styles_[category]['hatch_legend_linewidth'] = hl_lw
        linestyle = SafeGetStyle(styles, category, 'linestyle')
        if linestyle is None:
            linestyle = linestyles[i % len(linestyles)]
        styles_[category]['linestyle'] = linestyle
        marker = SafeGetStyle(styles, category, 'marker')
        if marker is None:
            marker = markers[i % len(markers)]
        styles_[category]['marker'] = marker

    # If user only wants a legend, build legend proxies now and return.
    if legend_only:
        legend_handles = []
        legend_labels = []
        for category in time_categories:
            s = styles_[category]
            lh = Line2D([0],[0], color=s['color'], linestyle=s['linestyle'], marker=s['marker'], linewidth=2, markersize=6)
            legend_hatch = (s.get('hatch') or '') * int(s.get('hatch_legend_repeat', 1))
            legend_hatch_lw = s.get('hatch_legend_linewidth', 1.2)
            try:
                legend_hatch_lw = float(legend_hatch_lw)
            except Exception:
                legend_hatch_lw = 1.2
            bh = Patch(facecolor="none", edgecolor=s['color'], hatch=legend_hatch, linewidth=legend_hatch_lw, alpha=0.99)#edgecolor=to_rgba('black', 0.8))
            legend_handles.append((lh, bh))
            legend_labels.append(category)

        ax.axis('off')
        target = ax
        target.legend(legend_handles, legend_labels, handler_map={tuple: HandlerTuple(ndivide=2)},
                      title=legend_title,
                      loc=legend_loc,
                      ncols=legend_cols,
                      bbox_to_anchor=(legend_bbox_to_anchor if legend_bbox_to_anchor is not None else None),
                      frameon=False)
        if file_out is not None:
            plt.savefig(f"{file_out}", bbox_inches='tight', pad_inches=0.1)
        plt.show()
        return fig, ax

    for i, (category, percentage_column) in enumerate(zip(time_categories, percentage_columns)):
        all_x = df_a[dep_var].to_numpy(dtype=float)
        all_y = df_a[percentage_column].to_numpy(dtype=float)

        x = summary[dep_var].to_numpy(dtype=float)
        y = summary[percentage_column].to_numpy(dtype=float)
        y_lo = lo[percentage_column].to_numpy(dtype=float)
        y_hi = hi[percentage_column].to_numpy(dtype=float)
        s = styles_[category]
        l = ax.plot(x,y, label=category, alpha=0.9, color=s['color'], linestyle=s['linestyle'])
        if plot_points=='all':
            ax.scatter(all_x, all_y, alpha=0.9, color=s['color'], marker=s['marker'])
        if plot_points=='mean':
            ax.scatter(x, y, alpha=0.9, color=s['color'], marker=s['marker'])

        if not ci is None:
            # Use repeated hatch pattern and optional linewidth for better visibility
            hatch_pattern = (s.get('hatch') or '') * int(s.get('hatch_graph_repeat', 1))
            hb_kwargs = dict(alpha=0.18, color=s['color'], hatch=hatch_pattern)
            hg_lw = s.get('hatch_graph_linewidth')
            if hg_lw is not None:
                try:
                    hb_kwargs['linewidth'] = float(hg_lw)
                except Exception:
                    pass
            ax.fill_between(x, y_lo, y_hi, **hb_kwargs)

    legend_handles = []
    legend_labels = []
    for category in time_categories:
        s = styles_[category]
        lh = Line2D([0],[0], color=s['color'], linestyle=s['linestyle'],marker=s['marker'],linewidth=2,markersize=6)
        legend_hatch = (s.get('hatch') or '') * int(s.get('hatch_legend_repeat', 1))
        legend_hatch_lw = s.get('hatch_legend_linewidth', 1.2)
        try:
            legend_hatch_lw = float(legend_hatch_lw)
        except Exception:
            legend_hatch_lw = 1.2
        bh = Patch(facecolor="none", edgecolor=s['color'], hatch=legend_hatch, linewidth=legend_hatch_lw)
        legend_handles.append((lh, bh))
        legend_labels.append(category)

    ax.legend(legend_handles, legend_labels, handler_map={tuple: HandlerTuple(ndivide=2)},
                title=legend_title, 
                loc=legend_loc,
                ncols=legend_cols,
                bbox_to_anchor=legend_bbox_to_anchor)
    
    if title is None:
        title = f"Mean CPU time distribution vs {dep_var}\n {config_name}"
    ax.set_title(title)
    if dep_var_label is None:
        dep_var_label = dep_var
    ax.set_xlabel(dep_var_label)
    if y_label is None:
        y_label = "\\% of CPU time"
    ax.set_ylabel(y_label)

    ax.set_ylim(0, 100)
    x_values = summary[dep_var].dropna().to_numpy(dtype=float)
    if ticks_for_existing_values_only:
        ax.set_xticks(sorted(np.unique(x_values)))
    else:
        integer_x = x_values.size > 0 and np.allclose(x_values, np.round(x_values))
        ax.xaxis.set_major_locator(MaxNLocator(nbins=8, integer=integer_x))
        ax.yaxis.set_major_locator(MaxNLocator(nbins=8))

    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    if (not file_out is None):
        if 'png' in file_out:
            plt.savefig(f"{file_out}", dpi=300)
        else:
            plt.savefig(f"{file_out}")

    plt.show()

def plot_experiment_spread(
    data,
    configurations=None,
    independent_var="Horizon",
    dependent_var="Total Time",
    config_labels=None,
    title=None,
    ind_label=None,
    dep_label=None,
    confidence=0.95,
    show_prediction_band=True,
    jitter=0.04,
    log_scale=True,
    colors=None,
    alpha=0.45,
    point_size=28,
    fig_size=(10, 8),
    file_out=None,
):
    """Plot raw experiments, exponential fits, prediction bands and residuals.

    The model y = a * exp(b * x) is fitted separately for each configuration
    by ordinary least squares on log(y). The shaded area is a pointwise
    prediction interval for an individual observation, not a confidence
    interval for the mean, so it visualises expected run-to-run spread.
    If SciPy is unavailable, the interval uses a normal critical-value
    approximation instead of Student's t critical value.

    Repeated x values are spread horizontally for display only. The fit and
    returned residuals always use the original x values. Pass raw, unaveraged
    results (plot_df in the Search Rescue notebook), not average_times.

    Returns
    -------
    fig, axes, fit_results, residual_data
        axes contains the fit and log-residual plots. fit_results has one row
        per configuration; residual_data has one row per raw observation.
    """
    required = {"Configuration", independent_var, dependent_var}
    missing = sorted(required.difference(data.columns))
    if missing:
        raise KeyError(f"Data frame is missing columns: {', '.join(missing)}")
    if not 0 < confidence < 1:
        raise ValueError("confidence must be strictly between 0 and 1.")
    if jitter < 0:
        raise ValueError("jitter must be non-negative.")

    plot_data = data.loc[:, list(required)].copy()
    for column in (independent_var, dependent_var):
        plot_data[column] = pd.to_numeric(plot_data[column], errors="coerce")

    if configurations is None:
        configurations = list(plot_data["Configuration"].drop_duplicates())
    else:
        configurations = list(configurations)
        plot_data = plot_data[plot_data["Configuration"].isin(configurations)]

    if isinstance(config_labels, dict):
        labels = {c: config_labels.get(c, c) for c in configurations}
    elif config_labels is None:
        labels = {c: c for c in configurations}
    else:
        config_labels = list(config_labels)
        if len(config_labels) != len(configurations):
            raise ValueError("config_labels must have one label per configuration.")
        labels = dict(zip(configurations, config_labels))

    valid = (
        np.isfinite(plot_data[independent_var])
        & np.isfinite(plot_data[dependent_var])
        & (plot_data[dependent_var] > 0)
    )
    valid_data = plot_data[valid].copy()
    if valid_data.empty:
        raise ValueError("No finite observations with positive y values are available.")

    unique_x = np.sort(valid_data[independent_var].unique())
    spacings = np.diff(unique_x)
    spacings = spacings[spacings > 0]
    jitter_width = jitter * (spacings.min() if spacings.size else 1.0)
    colors = {} if colors is None else colors

    fig, axes = plt.subplots(
        2, 1, figsize=fig_size, sharex=True,
        gridspec_kw={"height_ratios": [3, 1]}, constrained_layout=True,
    )
    fit_ax, residual_ax = axes
    fit_records = []
    residual_frames = []

    for configuration in configurations:
        configuration_data = valid_data[
            valid_data["Configuration"] == configuration
        ].sort_values(independent_var, kind="stable")
        if configuration_data.empty:
            continue

        x = configuration_data[independent_var].to_numpy(dtype=float)
        y = configuration_data[dependent_var].to_numpy(dtype=float)
        if len(x) < 2 or np.unique(x).size < 2:
            continue

        log_y = np.log(y)
        growth_rate, log_scale = np.polyfit(x, log_y, deg=1)
        fitted_log_y = log_scale + growth_rate * x
        fitted_y = np.exp(fitted_log_y)
        log_residuals = log_y - fitted_log_y

        jittered_x = x.copy()
        if jitter_width:
            for x_value in np.unique(x):
                positions = np.flatnonzero(x == x_value)
                if len(positions) > 1:
                    jittered_x[positions] += np.linspace(
                        -jitter_width, jitter_width, len(positions)
                    )

        scatter = fit_ax.scatter(
            jittered_x, y, s=point_size, alpha=alpha,
            color=colors.get(configuration), label=labels[configuration],
            edgecolors="none",
        )
        color = scatter.get_facecolor()[0]
        curve_x = np.linspace(x.min(), x.max(), 300)
        curve_y = np.exp(log_scale + growth_rate * curve_x)
        fit_ax.plot(curve_x, curve_y, color=color, linewidth=2.2)

        n = len(x)
        degrees_of_freedom = n - 2
        sum_squared_x = np.sum((x - x.mean()) ** 2)
        lower_y = np.full(n, np.nan)
        upper_y = np.full(n, np.nan)
        coverage = np.nan
        rmse_log = np.sqrt(np.mean(log_residuals ** 2))
        interval_method = None

        if degrees_of_freedom > 0 and sum_squared_x > 0:
            residual_standard_error = np.sqrt(
                np.sum(log_residuals ** 2) / degrees_of_freedom
            )
            interval_method = "student_t"
            if student_t is None:
                critical_t = NormalDist().inv_cdf(0.5 + confidence / 2)
                interval_method = "normal_approximation"
            else:
                critical_t = student_t.ppf(0.5 + confidence / 2, degrees_of_freedom)

            def prediction_limits(values):
                values = np.asarray(values, dtype=float)
                predicted_log = log_scale + growth_rate * values
                prediction_se = residual_standard_error * np.sqrt(
                    1 + 1 / n + ((values - x.mean()) ** 2) / sum_squared_x
                )
                return (
                    np.exp(predicted_log - critical_t * prediction_se),
                    np.exp(predicted_log + critical_t * prediction_se),
                )

            lower_y, upper_y = prediction_limits(x)
            coverage = np.mean((y >= lower_y) & (y <= upper_y))
            if show_prediction_band:
                curve_lower, curve_upper = prediction_limits(curve_x)
                fit_ax.fill_between(
                    curve_x, curve_lower, curve_upper,
                    color=color, alpha=0.14, linewidth=0,
                )

        residual_ax.scatter(
            jittered_x, log_residuals, s=point_size, alpha=alpha,
            color=color, edgecolors="none",
        )

        original_residuals = y - fitted_y
        original_total_ss = np.sum((y - y.mean()) ** 2)
        log_total_ss = np.sum((log_y - log_y.mean()) ** 2)
        fit_records.append({
            "Configuration": configuration,
            "Coefficient_a": np.exp(log_scale),
            "Coefficient_b": growth_rate,
            "R_squared_log": (
                np.nan if np.isclose(log_total_ss, 0)
                else 1 - np.sum(log_residuals ** 2) / log_total_ss
            ),
            "R_squared_original": (
                np.nan if np.isclose(original_total_ss, 0)
                else 1 - np.sum(original_residuals ** 2) / original_total_ss
            ),
            "RMSE_log": rmse_log,
            "Geometric_RMSE_factor": np.exp(rmse_log),
            "Prediction_interval_coverage": coverage,
            "Prediction_interval_method": interval_method,
            "Number_of_points": n,
        })
        residual_frames.append(pd.DataFrame({
            "Configuration": configuration,
            independent_var: x,
            "Observed": y,
            "Fitted": fitted_y,
            "Log_residual": log_residuals,
            "Relative_residual_percent": 100 * (y / fitted_y - 1),
            "Prediction_interval_lower": lower_y,
            "Prediction_interval_upper": upper_y,
        }, index=configuration_data.index))

    if not fit_records:
        plt.close(fig)
        raise ValueError(
            "At least one configuration needs observations at two distinct x values."
        )

    fit_ax.set_title(
        title or f"{dependent_var} spread and exponential fit against {independent_var}"
    )
    fit_ax.set_ylabel(dep_label or dependent_var)
    fit_ax.grid(True, alpha=0.3)
    fit_ax.legend(title="Configuration", loc="best")
    if log_scale:
        fit_ax.set_yscale("log")
        fit_ax.set_ylabel(f"{dep_label or dependent_var} (log scale)")

    residual_ax.axhline(0, color="0.25", linestyle="--", linewidth=1.2)
    residual_ax.set_xlabel(ind_label or independent_var)
    residual_ax.set_ylabel("log(observed / fitted)")
    residual_ax.grid(True, alpha=0.3)

    if file_out is not None:
        fig.savefig(file_out, bbox_inches="tight")
    plt.show()

    return (
        fig,
        axes,
        pd.DataFrame(fit_records),
        pd.concat(residual_frames).sort_index(),
    )
