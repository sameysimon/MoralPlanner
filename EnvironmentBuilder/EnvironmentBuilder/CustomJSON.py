import json
def custom_json_format(obj, indent=0):
    """
    Recursively format the JSON output:
    - Lists are printed on one line.
    - Dictionary elements are spread across multiple lines.
    """
    # Handle dictionaries
    if isinstance(obj, dict):
        if (len(obj)==0):
            return "{}"
        items = []
        for key, value in obj.items():
            formatted_value = custom_json_format(value, indent + 4)
            items.append(f'{" " * (indent + 4)}"{key}": {formatted_value}')
        return "{\n" + ",\n".join(items) + "\n" + " " * indent + "}"

    # Handle lists
    elif isinstance(obj, list):
        formatted_list = ", ".join(custom_json_format(item, indent) for item in obj)
        return f"[{formatted_list}]"

    # Use standard JSON formatting for other types
    else:
        return json.dumps(obj)