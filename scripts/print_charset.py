def unique_characters(file_path):
    unique_chars = set()
    
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            for char in line:
                if not char.isspace():  # Exclude whitespace characters
                    unique_chars.add(char)
    
    # Convert the set to a sorted list and then join to form a string
    unique_chars_string = ''.join(sorted(unique_chars))
    
    return unique_chars_string

if __name__ == "__main__":
    file_path = '../source_net_common/net_assets/data/words.csv'
    unique_chars_string = unique_characters(file_path)
    print(unique_chars_string)
