import argparse

def hex_to_bin(hex_file, bin_file, pad_byte=0xFF):
    """
    Converts an Intel HEX file to a binary file.

    Args:
        hex_file (str): The path to the input hex file.
        bin_file (str): The path to the output binary file.
        pad_byte (int): Padding byte value (default: 0xFF)
    """
    try:
        memory = {}
        lowest_address = float('inf')
        highest_address = 0
        base_address = 0

        # First pass: Find address range
        with open(hex_file, 'r') as infile:
            for line in infile:
                line = line.strip()
                if not line or not line.startswith(':'):
                    continue

                data = bytes.fromhex(line[1:])
                byte_count = data[0]
                address = (data[1] << 8) | data[2]
                record_type = data[3]

                if record_type == 0:  # Data Record
                    phys_addr = base_address + address
                    if byte_count > 0:
                        if phys_addr < lowest_address:
                            lowest_address = phys_addr
                        if phys_addr + byte_count - 1 > highest_address:
                            highest_address = phys_addr + byte_count - 1
                elif record_type == 4:  # Extended Linear Address
                    base_address = (data[4] << 24) | (data[5] << 16)

        # Initialize memory array
        memory = bytearray([pad_byte] * (highest_address - lowest_address + 1))

        # Second pass: Fill in data
        base_address = 0
        with open(hex_file, 'r') as infile:
            for line in infile:
                line = line.strip()
                if not line or not line.startswith(':'):
                    continue

                data = bytes.fromhex(line[1:])
                byte_count = data[0]
                address = (data[1] << 8) | data[2]
                record_type = data[3]

                if record_type == 0:  # Data Record
                    phys_addr = base_address + address - lowest_address
                    memory[phys_addr:phys_addr + byte_count] = data[4:4 + byte_count]
                elif record_type == 4:  # Extended Linear Address
                    base_address = (data[4] << 24) | (data[5] << 16)

        with open(bin_file, 'wb') as outfile:
            outfile.write(memory)

        print(f"Successfully converted {hex_file} to {bin_file}")

    except FileNotFoundError:
        print(f"Error: Input file {hex_file} not found.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert a hex file to a binary file.")
    parser.add_argument("hex_file", help="Path to the input hex file")
    parser.add_argument("bin_file", help="Path to the output binary file")
    args = parser.parse_args()

    hex_to_bin(args.hex_file, args.bin_file)