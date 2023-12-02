def file_copy_and_replace(source_path, target_path, replacement_pairs):
    source_file = open(source_path, 'r')
    target_file = open(target_path, 'w')

    for line in source_file.readlines():
        output_line = line
        for pair in replacement_pairs:
            search, replacement = pair
            output_line = output_line.replace(search, replacement)
        target_file.write(output_line)
    target_file.close()


def is_lowercase(string):
    for letter in string:
        if letter.isupper():
            return False
    return True


def to_snake_case(string):
    snake = ''
    score = False
    for letter in string:
        if letter.isupper():
            if score:
                snake += '_' + letter.lower()
            else:
                snake += letter.lower()
        else:
            snake += letter
        score = True
    return snake


def to_camel_case(string):
    camel = ''
    upper = True
    for letter in string:
        if letter == '_':
            upper = True
        else:
            if upper:
                camel += letter.upper()
                upper = False
            else:
                camel += letter
    return camel
