#!/usr/bin/env python3
"""
Scanning the TCP/IP host to find vulnerable port for attack
Port 0 - 10000 of the host will be scanned...
"""

import argparse
import socket


class PortScanner:
    def __init__(self, host, port):
        self.host = host
        self.ports = port
        self.output = {
            'open': [],
            'close': [],
            'error': [],
        }

    def setTimeout(self, timeout):
        # set longer timeout in case the Internet is unstable
        socket.setdefaulttimeout(timeout)

    def connect(self):
        # try:
            # addr = socket.gethostbyname(self.host)
            # print(addr)
        print('Scanning {} ports at {}, from #{} to #{}...'.format(
                len(self.ports), self.host, self.ports[0], self.ports[-1]))
        # except:
        #     print("Error - Cannot resolve '{}': Unknown host".format(self.host))
        #     return False

        # start parallel processing of scanning all target ports
        for port in self.ports:
            print(' + scanning port # {}'.format(port))
            self.scan(port)

        return True

    def scan(self, port):
        try:
            connSkt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            connSkt.connect((self.host, port))
            connSkt.send(b'test\r\n')
            results = connSkt.recv(100)

            self.output['open'].append((port, results))

        except ConnectionRefusedError as e:
            self.output['close'].append(port)

        except OSError as e:
            self.output['error'].append((port, e))

        finally:
            connSkt.close()

    def report(self):
        # report the result if success
        try:
            # resolve the host ip address
            addr = socket.gethostbyname(target_host)
            name = socket.gethostbyaddr(addr)[0]
        except:
            name = ''
        print('\nPort Scan Results for: {}({})'.format(name, addr))

        report = '\nScanned {} ports\n'.format(
            len(self.output['open']) + len(self.output['close']) + len(self.output['error'])
        )

        report += '\n  Open Ports x {}\n'.format(len(self.output['open']))
        for opened in self.output['open']:
            report += '    {}/tcp open\n      {}\n'.format(opened[0], opened[1])

        report += '\n  Close Ports x {}\n'.format(len(self.output['close']))

        report += '\n  Error Ports x {}\n'.format(len(self.output['error']))
        for err in self.output['error']:
            report += '    {}/tcp error\n      {}\n'.format(err[0], err[1])

        print(report)


if __name__ == '__main__':

    # parse from command line the target host location and port to be scanned
    parser = argparse.ArgumentParser(description='scan a host for open ports')
    parser.add_argument('-i', '--host', default='localhost',
                        help='target host location or id address')
    parser.add_argument('-p', '--port', type=int, nargs='*', default='1000',
                        help='target max port number to be scanned (from 1 - max)')
    parser.add_argument('-t', '--timeout', type=int, default=10,
                        help='timeout for port scan until error')
    args = vars(parser.parse_args())

    # define the variables based on parsed arguments
    target_host = args['host']
    if len(args['port']) == 1:
        target_ports = range(1, args['port'][0])
    else:
        target_ports = range(args['port'][0], args['port'][1])

    # setup the port scanner
    scanner = PortScanner(target_host, target_ports)
    scanner.setTimeout(args['timeout'])

    # start the port scanning
    if scanner.connect():
        scanner.report()
