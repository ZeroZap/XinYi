import argparse
import struct

def mot2bin(mot_file, bin_file, pad_byte=0xFF):
    """
    Converts a Motorola S-record file to a binary file.

    Args:
        mot_file (str): The path to the input Motorola S-record file.
        bin_file (str): The path to the output binary file.
        pad_byte (int): Byte value used for padding (default: 0xFF)
    """

    try:
        with open(mot_file, 'r') as infile, open(bin_file, 'wb') as outfile:
            data_dict = {}
            min_address = float('inf')
            max_address = 0

            for line in infile:
                line = line.strip()
                if not line.startswith('S'):
                    continue

                record_type = line[1]
                byte_count = int(line[2:4], 16)

                if record_type in ('0', '5', '7', '8', '9'):
                    continue

                if record_type in ('1', '2', '3'):
                    if record_type == '1':
                        address = int(line[4:8], 16)
                        data = bytes.fromhex(line[8:-2])
                    elif record_type == '2':
                        address = int(line[4:10], 16)
                        data = bytes.fromhex(line[10:-2])
                    elif record_type == '3':
                        address = int(line[4:12], 16)
                        data = bytes.fromhex(line[12:-2])

                    checksum = int(line[-2:], 16)
                    calculated_checksum = 0
                    for byte in bytes.fromhex(line[2:-2]):
                        calculated_checksum += byte
                    calculated_checksum = 255 - (calculated_checksum & 0xFF)

                    if calculated_checksum != checksum:
                        print(f"Checksum error in line: {line}")
                        continue

                    data_dict[address] = data
                    min_address = min(min_address, address)
                    max_address = max(max_address, address + len(data) - 1)

            # Create binary data with padding
            binary_data = bytearray()
            for i in range(min_address, max_address + 1):
                if i in data_dict:
                    binary_data.extend(data_dict[i])
                else:
                    binary_data.append(0xFF)  # Pad with 0xFF

            outfile.write(binary_data)

            print(f"Successfully converted {mot_file} to {bin_file}")

    except FileNotFoundError:
        print(f"Error: Input file {mot_file} not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert a Motorola S-record file to a binary file.")
    parser.add_argument("mot_file", help="Path to the input Motorola S-record file")
    parser.add_argument("bin_file", help="Path to the output binary file")
    parser.add_argument("--pad", type=lambda x: int(x,0), default=0xFF,
                      help="Padding byte value (default: 0xFF)")
    args = parser.parse_args()

    mot2bin(args.mot_file, args.bin_file, args.pad)
    mot2bin(args.mot_file, args.bin_file)