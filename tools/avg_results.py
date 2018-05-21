#!/usr/bin/env python3
import subprocess

import argparse
import csv


EXECUTABLE_Q = '/usr/bin/q-text-as-data'
CSV_DELIMITER = ','


from pprint import pprint

def parse_args():
    parser = argparse.ArgumentParser(
        description='Will build the average over an input csv file and write it to an other csv',
        prog='avg_results'
    )

    parser.add_argument(
        '-i',
        type=str,
        nargs='?',
        dest='infiles',
        required=True,
        help='Path to infile'
    )
    
    parser.add_argument(
        '-o',
        type=str,
        nargs='?',
        dest='outfile',
        required=True,
        help='Path to outfile'
    )

    parser.add_argument(
        '-a',
        type=str,
        nargs='?',
        dest='attributes',
        required=False,
        help='Additional attributes to append in format attribute1=value1,attribute2=value2'
    )

    parser.add_argument(
        '--average',
        type=str,
        nargs='?',
        dest='average',
        required=True,
        help='Fields being averaged'
    )

    parser.add_argument(
        '--ignore-in-groupby',
        type=str,
        nargs='?',
        dest='ignoregb',
        required=False,
        help='Fields not being included in the GROUP BY statement'
    )

    parser.add_argument(
        '--s',
        action='store_true',
        dest='source_indic',
        help='Add a column in the result indicating the source file'
    )

    return parser.parse_args()

def get_csv_header(path: str):
    with open(path, "r") as f:
        return [item for item in csv.reader(f, delimiter=CSV_DELIMITER)][0]

def write_raw(path: str, content: list):
    with open(path, 'w') as f:
        f.writelines(content)


def generate_sql(header: list, infiles: list):
    avg = set(args.average.split(','))
    groupby = set(header) - avg
    if args.ignoregb:
        groupby = groupby - set(args.ignoregb.split(','))
    if args.attributes:
            header.extend(["'{0[1]}' AS {0[0]}".format(item.split('=')) for item in args.attributes.split(',')])
    avg = set(args.average.split(','))
    select = set(header) - avg
    select.update(['avg({})'.format(item) for item in avg])
    select = sorted(select)
    header = [item.replace('(', '_').replace(')', '') for item in select]
    return  ['SELECT {select} FROM {infile} GROUP BY {gb}'.format(select=', '.join(select), infile=infile, gb=', '.join(groupby)) for infile in infiles], header

def invoke_query_tool(sql: str):
    result = subprocess.run(
        [EXECUTABLE_Q, '-d', CSV_DELIMITER, '-H', sql],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    if result.returncode != 0:
        print(result.stderr)
    else:
        return result.stdout.decode('utf8')



def init():
    global args
    args = parse_args()

def main():
    infiles = args.infiles.split(',')
    header = get_csv_header(infiles[0])
    statements, header = generate_sql(header, infiles)
    raw_out = ''
    for sql in statements:
        raw_out += '\n' + invoke_query_tool(sql)
    write_raw(args.outfile, CSV_DELIMITER.join(header) + raw_out)


if __name__ == '__main__':
    init()
    main()