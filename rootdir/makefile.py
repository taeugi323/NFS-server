f = open("input_file.txt", 'w')

input_str = ""

# 4KB string
for k in range(4):
  for j in range(32):
    for i in range(31):
      input_str += 'a'
    input_str += '\n'

# make 4GB file
for i in range(1024*4):
  f.write(input_str)


f.close()
