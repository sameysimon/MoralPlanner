import networkx as nx
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from copy import deepcopy

def BuildNxGraph(adj_edge):
    G = nx.DiGraph()
    for src, targets in adj_edge.items():
        src = int(src)

        G.add_node(src)

        for dst in targets:
            dst = int(dst)
            G.add_edge(src, dst)

    return G

def _DrawGraphOnAxes(ax, G, pos,
                    curr_props=None, 
                    community=None, 
                    visited=None,
                    traversed_edges=None,
                    current_action=None, current_worth=None, cumulative_worth=None):
    if curr_props is None:
        current_node = None
    else:
        current_node = curr_props['curr_tile']


    if visited is None:
        visited = set()

    if traversed_edges is None:
        traversed_edges = []

    ax.clear()

    node_colours = []

    for node in G.nodes:
        if community is not None:
            
            colour = community[int(node)]

            if colour == "":
                colour = "grey"

            node_colours.append(colour)
        else:
            if node in visited:
                node_colours.append("orange")
            else:
                node_colours.append("lightblue")

    nx.draw_networkx_nodes(
        G,
        pos,
        node_color=node_colours,
        node_size=800,
        ax=ax,
    )

    if current_node is not None:
        nx.draw_networkx_nodes(
            G,
            pos,
            nodelist=[current_node],
            node_color="none",
            edgecolors="black",
            linewidths=3,
            node_size=1100,
            ax=ax,
        )

    node_labels = {}
    for node in G.nodes:
        node_labels[node] = curr_props['tile_type'][int(node)]
        node_labels[node] = node_labels[node].replace("hospital", "H")
        nx.draw_networkx_labels(
            G,
            pos,
            labels=node_labels,
            font_size=12,
            font_weight="bold",
            font_color="white",
            ax=ax
        )
    

    nx.draw_networkx_edges(
        G,
        pos,
        arrows=True,
        arrowstyle="-|>",
        arrowsize=25,
        width=1.5,
        connectionstyle="arc3,rad=0.08",
        ax=ax,
    )

    # Highlight traversed path
    if traversed_edges:
        nx.draw_networkx_edges(
            G,
            pos,
            edgelist=traversed_edges,
            arrows=True,
            arrowstyle="-|>",
            arrowsize=27,
            width=3,
            edge_color="purple",
            connectionstyle="arc3,rad=0.08",
            ax=ax,
        )
    if current_node is not None:
        ax.set_title(f"Node {current_node} at time {curr_props['time']}")
    ax.axis("off")

    t = ""
    if current_action is not None:
        t += f"Agent executes {current_action}\n"
    if current_worth is not None:
        t += f"{current_worth}\n"
    if cumulative_worth is not None:
        t+= f"cumulative {cumulative_worth}\n"
    if curr_props is not None:
        t+= f"Holding:{curr_props["holding"]}"
    if t != "":
        ax.text(0.5,-0.08, t ,transform=ax.transAxes, ha="center", va="top", fontsize=12, wrap=True)
        plt.subplots_adjust(bottom=0.18)

def DrawGraph(adj_edge, community=None, curr_props=None, visited=None):
    G = BuildNxGraph(adj_edge)
    current_node = curr_props['curr_tile']

    pos = nx.bfs_layout(G, start=current_node)

    fig, ax = plt.subplots(figsize=(8, 6))
    _DrawGraphOnAxes(
        ax=ax,
        G=G,
        pos=pos,
        curr_props=curr_props,
        community=community,
        visited=visited
    )

    plt.show()

def AnimateAgent(adj_edge, scr_history=None, scr_tag_sequence=None, community=None, action_history=None, worth_history=None, cumulative_worth=None, interval=700):
    G = BuildNxGraph(adj_edge)

    agent_path = []
    for i in range(len(scr_history)):
        agent_path.append(scr_tag_sequence[i]["curr_tile"])
    agent_path.append(scr_history[-1][1])

    # Compute layout
    start_node = agent_path[0]
    pos = nx.bfs_layout(G, start=start_node)

    fig, ax = plt.subplots(figsize=(8, 6))

    def update(frame):

        visited = set(agent_path[:frame + 1])

        traversed_edges = list(
            zip(agent_path[:frame], agent_path[1:frame + 1])
        )
        curr_action = "None"
        curr_props = None
        curr_worth = "None"
        cum_worth = "None"
        if (frame < len(action_history)):
            curr_action = action_history[frame]
            curr_worth = worth_history[frame]
            curr_props = scr_tag_sequence[frame]
        else:
            curr_worth = worth_history[-1]

        if (frame >= len(cumulative_worth)):
            cum_worth = cumulative_worth[-1]
        else:
            cum_worth = cumulative_worth[frame]

        _DrawGraphOnAxes(
            ax=ax,
            G=G,
            pos=pos,
            community=community,
            curr_props=curr_props,
            visited=visited,
            traversed_edges=traversed_edges,
            current_action=curr_action,
            current_worth=curr_worth,
            cumulative_worth=cum_worth,
        )

    ani = FuncAnimation(
        fig,
        update,
        frames=len(agent_path),
        interval=interval,
        repeat=False,
    )

    plt.show()

    return ani