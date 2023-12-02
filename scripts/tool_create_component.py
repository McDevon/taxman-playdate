import argparse
import os
from tool_templates import is_lowercase, to_camel_case, to_snake_case, file_copy_and_replace


def create_component_file(snake_name, camel_name, templates_path, output_directory):
    header_path = os.path.join(templates_path, 'template_component.h')
    if not os.path.isfile(header_path):
        print(f'{header_path} is not a file, cannot create component')
        return

    source_path = os.path.join(templates_path, 'template_component.c')
    if not os.path.isfile(source_path):
        print(f'{source_path} is not a file, cannot create component')
        return

    file_copy_and_replace(header_path, os.path.join(output_directory, snake_name + '.h'),
                          [('template_component', snake_name), ('TemplateComponent', camel_name)])
    file_copy_and_replace(source_path, os.path.join(output_directory, snake_name + '.c'),
                          [('template_component', snake_name), ('TemplateComponent', camel_name)])


def main():
    parser = argparse.ArgumentParser(
        description='Create empty component class from template',
    )
    parser.add_argument('name', type=str,
                        help='name of the component in either CamelCase or snake_case')

    def parse_error(error):
        print(error + '\n')
        parser.print_help()

    parser.error = parse_error

    args = parser.parse_args()

    component_name = args.name

    snake_name = ''
    camel_name = ''

    if is_lowercase(component_name):
        snake_name = component_name
        camel_name = to_camel_case(component_name)
    else:
        snake_name = to_snake_case(component_name)
        camel_name = to_camel_case(component_name)

    create_component_file(snake_name, camel_name, 'scripts/Templates/', 'game')

    return 0


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e)
