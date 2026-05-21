import re
import pandas as pd

def SaveDataFrameToTexTemplate(df:pd.DataFrame, template_file:str, output_file:str, row_template_mode:bool=True):
    with open(template_file, encoding='utf-8') as f:
        content = f.read()
    
    if row_template_mode:
        final_content = TemplateMode(df, content)
    else:
        final_content = FindReplaceMode(df, content)
    
    with open(output_file, 'x') as f:
        f.write(final_content)
    print(f"Successfully saved populated LaTeX to {output_file}")

def FindReplaceMode(df:pd.DataFrame, content:str):
    c = f"{content}"
    for _, df_row in df.iterrows():
        for col_idx in range(len(df.columns)):
            placeholder = f"§{col_idx}"
            real_val = str(df_row.iloc[col_idx])
            c = re.sub(placeholder, real_val, c, count=1)
    return c

def TemplateMode(df:pd.DataFrame, content:str):
    rows = content.split(r'\\')
    template_row_index = -1
    template_row_index = len(rows) - 2
    if template_row_index < 0:
        print("Not enough rows.")
        return
    
    template_row = rows[template_row_index]
    new_rows = []

    for _, df_row in df.iterrows():
        current_row = template_row
        for col_idx in range(len(df.columns)):
            placeholder = f"§{col_idx}"
            current_row = re.sub(placeholder, str(df_row.iloc[col_idx]), current_row)
        new_rows.append(current_row)
    
    rows[template_row_index : template_row_index + 1] = new_rows
    return "\\\\".join(rows)
