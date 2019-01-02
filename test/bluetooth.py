# Capture incoming serial data and display ppm values
import serial
# from programSettings import programSettings
# import list_ports
# import cmd
import sys

BT_PPM_STARTCHAR = '$'

if len(sys.argv) == 1:
    print("""
    Usage:
        bluetooth.py <serialPort> <baud>

        Reads ppm values from <serialPort> and displays them
        """)
else:
    # Arguments
    baudrateStr = sys.argv[2]
    serialPortStr = sys.argv[1]
    # Setup port
    port = serial.Serial()
    port.baudrate = int(baudrateStr)
    port.port = serialPortStr
    # port.timeout = 0  # nonblocking
    port.open()

    print("Port open, starting reads")
    print("Ailr\tElev\tThrt\tRddr\tAux1\tAux2\tAux3")

    while 1:
        # Loop over serial data
        data = port.read()
        if data == BT_PPM_STARTCHAR:
            packet = bytearray()
            packet.extend(data)  # start char at [0]
            for i in xrange(1,17):
                rcvd = port.read()
                if rcvd:
                    packet.extend(rcvd)
                    # print(format(packet[i], '#04x'))
            # aileron = packet[3]<<8 + packet[2]
            aileron = (packet[3] << 8) + packet[2]
            elevator = (packet[5] << 8) + packet[4]
            throttle = (packet[7] << 8) + packet[6]
            rudder = (packet[9] << 8) + packet[8]
            aux1 = (packet[11] << 8) + packet[10]
            aux2 = (packet[13] << 8) + packet[12]
            aux3 = (packet[15] << 8) + packet[14]
            sys.stdout.write(str(aileron) + "\t" +
                             str(elevator) + "\t" +
                             str(throttle) + "\t" +
                             str(rudder) + "\t" +
                             str(aux1) + "\t" +
                             str(aux2) + "\t" +
                             str(aux3) + "\t" +
                             "\r")
            sys.stdout.flush()

    port.close()




# class bluetoothCommandLine(cmd.Cmd):

#     """Command Line Interface"""

#     prompt = "> "
#     settings = programSettings()

#     """Default/Required Commands"""

#     def do_EOF(self, line):
#         return True

#     def do_exit(self, line):
#         """Exit Program"""
#         return True

#     def do_quit(self, line):
#         """Exit Program"""
#         return True

#     """User Commands"""

#     """Run"""

#     def do_run(self, line):
#         """"""

#     """Settings"""

#     def do_list_settings(self, line):
#         """Lists current settings"""
#         print("Serial:\t\t" + self.settings.get_port() +
#               "\t" + self.settings.get_baud())

#     def do_set_port(self, port_name):
#         """do_set_port <port name> to set port;
#         prints available with no arg"""
#         if port_name:
#             self.settings.set_port(port_name)
#             print(port_name + " Selected")
#         else:
#             if self.settings.get_port():
#                 print("Currently selected port: " +
#                       self.settings.get_port())
#             ports = list_ports.list_serial_ports()
#             print("Select serial port to use:")
#             for port in ports:
#                 print(port)
#             print("Enter set_port <port name>")

#     def do_set_baud(self, baud_rate):
#         """set_baud <baud rate> to set baud rate"""
#         if baud_rate:
#             self.settings.set_baud(baud_rate)
#             print(baud_rate + " Set")
#         else:
#             if self.settings.get_baud():
#                 print("Currently selected baud: " +
#                       self.settings.get_baud())
#             print("Enter set_baud <baud rate>")


# if __name__ == '__main__':
#     import sys
#     if len(sys.argv) > 1:
#         bluetoothCommandLine().onecmd(' '.join(sys.argv[1:]))
#     else:
#         bluetoothCommandLine().cmdloop()
