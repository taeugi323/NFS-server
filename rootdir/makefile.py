f = open("input_file.txt", 'w')

input_str = ""
alphabet = [i for i in range(97,123)]

"""
# 4KB string
for k in range(4):
  for j in range(32):
    for i in range(31):
      input_str += 'a'
    input_str += '\n'
"""
# 1KB string
for j in range(64):
  for i in alphabet:
    input_str += str(unichr(i))
  input_str += "\n"

# make 100MB file
for i in range(1024*300):
  f.write(input_str)


f.close()
