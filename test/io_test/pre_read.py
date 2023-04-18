
dir = "merged/"

def test_read_many_pre():
    print("pre read 1g")
    file = open(dir + "file_2g", "rb+")
    for i in range(500000):
        pos = i*4096
        len = 400
        bs = b'a'*len
        file.seek(pos)
        if i%2==1:
            file.write(bs)
    file.close()
if __name__ == '__main__':
    test_read_many_pre()