import pytest

normal_dir="normal/"
overlay_dir="overlay/merged/"
source_file = None
def setup_module(module):
    source_file = open("source.txt", "rb")

# def teardown_module(module):
#     source_file.close()


def do_read_cmp(self, f1,f2,offset,len):
    f1.seek(offset)
    f2.seek(offset)
    data1 = f1.read(len)
    data2 = f2.read(len)
    assert data1==data2

def do_write(self, f1,f2,offset,len):
    f1.seek(offset)
    f2.seek(offset)
    data = source_file.read(len)
    f1.write(data)
    f2.write(data)

class TestS:
    normal_file = None
    overlay_file = None
    data = None
    def setup_class(self):
        normal_path = normal_dir + "s_file"
        overlay_path = overlay_dir + "s_file"
        normal_file = open(normal_path, "rb+")
        overlay_file = open(overlay_path, "rb+")
        print(normal_file)
        normal_file.seek(0)
        data = normal_file.read()
    
    def teardown_class(self):
        normal_file.truncate(0)
        normal_file.write(data)
        overlay_file.truncate(0)
        overlay_file.write(data)
        normal_file.close()
        overlay_file.close()

    # read_test
    def test_1(self):
        do_read_cmp(normal_file, overlay_file, 0, 10)
        do_read_cmp(normal_file, overlay_file, 10, 10)
        do_read_cmp(normal_file, overlay_file, 20, 100)
        do_read_cmp(normal_file, overlay_file, 120, 100)
        do_read_cmp(normal_file, overlay_file, 220, 1)
        do_read_cmp(normal_file, overlay_file, 9, 460)
    
    # read after read
    def test_2(self):
        do_write(normal_file, overlay_file, 5, 10)
        do_read_cmp(normal_file, overlay_file, 0, 20)
        do_write(normal_file, overlay_file, 15, 100)
        do_read_cmp(normal_file, overlay_file, 0, 120)
        do_write(normal_file, overlay_file, 115, 100)
        do_read_cmp(normal_file, overlay_file, 67, 220)
        do_write(normal_file, overlay_file, 215, 1)
        do_read_cmp(normal_file, overlay_file, 67, 220)

        