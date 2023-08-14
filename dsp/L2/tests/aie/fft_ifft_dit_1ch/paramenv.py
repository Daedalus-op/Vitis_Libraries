import json
import os
import sys
import subprocess
import re
from argparse import ArgumentParser

def runcommand(parameter_file, instance_name, var, command, connector = "=", action = "append"):
    command = re.sub(' +', ' ', command)
    f = open(parameter_file)
    if not f:
        print(parameter_file + " open failed")
        exit(1)
    parameters = json.load(f)
    params = parameters[instance_name]
    final_command = command
    if len(var) == 0:
        var = params.keys()
    if action == "append":
        envs = []
        for key, value in params.items():
            if key in var:
                envs.append(key + connector + value)
        env = " ".join(envs)
        final_command = command + " " + env
    elif action == "replace":
        for key, value in params.items():
            if key+connector in command and key in var:
                s_value = command.split(key + connector)[1]
                s_value = s_value.split(" ")[0]
                #print(key + " "+ value + " " + s_value)
                command = command.replace(key+connector+s_value, key+connector+value)
                #print("******" + command)
        final_command = command
    else:
        print(action + " is not supported")
        exit(1)
    print(final_command)
    process = subprocess.call(final_command, shell=True)
    return process

def main():
    parser = ArgumentParser()
    parser.add_argument('--parameter_file', type = str, required = True, help='instance paramter file')
    parser.add_argument('--instance_name',  type = str, required = True, help='instance name to run')
    parser.add_argument('--command',        type = str, required = True, help='the command will be run')
    parser.add_argument('--connector',      type = str, default = "=",   help='the charactor to connect var name and value')
    parser.add_argument('--action',         type = str, default = "append", choices = ['append','replace'], help='the charactor to connect var name and value')
    parser.add_argument('--var',         type = str, default = "", help='choose vars, sperated by comma')
    args = parser.parse_args()
    parameter_file = args.parameter_file
    instance_name = args.instance_name
    command = args.command
    connector = args.connector
    action = args.action
    if args.var:
        var = (args.var).split(":")
    else:
        var = []
    runcommand(parameter_file, instance_name,var, command, connector, action)

if __name__ == '__main__':
    main()
    
