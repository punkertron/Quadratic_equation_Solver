import random
import string

num_lines = 51225

def generate_random_value():
    if random.random() < 0.9:  # 90% chance to return an integer
        return str(random.randint(-10000, 10000))
    else:  # 10% chance to return a random garbage string
        length = random.randint(1, 4)
        return ''.join(random.choices(string.ascii_letters, k=length))

# Open a file to write the generated data
file_path = "test.data"
with open(file_path, "w") as file:
    for _ in range(num_lines):
        line = f"{generate_random_value()} {generate_random_value()} {generate_random_value()}\n"
        file.write(line)
