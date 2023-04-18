import pytest

normal_dir="normal/"
overlay_dir="overlay/merged/"
source_file = open("source.txt", "rb")
# def setup_module(module):
#     source_file = open("source.txt", "rb")

# def teardown_module(module):
#     source_file.close()


def do_read_cmp(f1,f2,offset,len):
    f1.seek(offset)
    f2.seek(offset)
    data1 = f1.read(len)
    data2 = f2.read(len)
    assert data1==data2

def do_write(f1,f2,offset,len,source_file):
    f1.seek(offset)
    f2.seek(offset)
    source_file.seek(0)
    data = source_file.read(len)
    f1.write(data)
    f2.write(data)

# self.normal_file = None
# self.overlay_file = None
# data = None

class TestS:
    def setup_class(self):
        normal_path = normal_dir + "s_file"
        overlay_path = overlay_dir + "s_file"
        self.source_file = source_file
        self.normal_file = open(normal_path, "rb+")
        self.overlay_file = open(overlay_path, "rb+")
        self.normal_file.seek(0)
        self.data = self.normal_file.read()
    
    def teardown_class(self):
        self.normal_file.truncate(0)
        self.normal_file.seek(0)
        self.normal_file.write(self.data)
        self.normal_file.close()
        self.overlay_file.close()

    # read_test
    def test_1(self):
        do_read_cmp(self.normal_file, self.overlay_file, 0, 10)
        do_read_cmp(self.normal_file, self.overlay_file, 10, 10)
        do_read_cmp(self.normal_file, self.overlay_file, 20, 100)
        do_read_cmp(self.normal_file, self.overlay_file, 120, 100)
        do_read_cmp(self.normal_file, self.overlay_file, 220, 1)
        do_read_cmp(self.normal_file, self.overlay_file, 9, 460)
    
    # read after read
    def test_2(self):
        do_write(self.normal_file, self.overlay_file, 5, 10, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 20)
        do_write(self.normal_file, self.overlay_file, 15, 100, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 120)
        do_write(self.normal_file, self.overlay_file, 115, 100, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 67, 222)
        do_write(self.normal_file, self.overlay_file, 215, 1, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 67, 220)
    
    
class TestM1:
    def setup_class(self):
        normal_path = normal_dir + "m_file1"
        overlay_path = overlay_dir + "m_file1"
        self.source_file = source_file
        self.normal_file = open(normal_path, "rb+")
        self.overlay_file = open(overlay_path, "rb+")
        self.normal_file.seek(0)
        self.data = self.normal_file.read()
    
    def teardown_class(self):
        self.normal_file.truncate(0)
        self.normal_file.seek(0)
        self.normal_file.write(self.data)
        self.normal_file.close()
        self.overlay_file.close()
    
    # read_test
    def test_1(self):
        do_read_cmp(self.normal_file, self.overlay_file, 0, 10)
        do_read_cmp(self.normal_file, self.overlay_file, 1000, 10)
        do_read_cmp(self.normal_file, self.overlay_file, 20, 14000)
        do_read_cmp(self.normal_file, self.overlay_file, 5020, 20000)
        do_read_cmp(self.normal_file, self.overlay_file, 360, 8900)
        do_read_cmp(self.normal_file, self.overlay_file, 10000,19999)
        do_read_cmp(self.normal_file, self.overlay_file, 0,39999)

    # read after read
    def test_2(self):
        do_write(self.normal_file, self.overlay_file, 1000, 9870, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 3, 20000)
        do_write(self.normal_file, self.overlay_file, 9000, 20000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 167, 28357)
        do_write(self.normal_file, self.overlay_file, 115, 30000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 67, 32993)
        do_write(self.normal_file, self.overlay_file, 2150, 19873, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file,2000, 20000)
        do_write(self.normal_file, self.overlay_file, 1000, 10, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 990, 30)
        do_write(self.normal_file, self.overlay_file, 20, 14000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 14020)
        do_write(self.normal_file, self.overlay_file, 5020, 20000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 40020)
        do_write(self.normal_file, self.overlay_file, 360, 8900, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 9260)
        do_write(self.normal_file, self.overlay_file, 10000,19999, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 29999)
        do_write(self.normal_file, self.overlay_file, 0,39999, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 39999)

class TestM3:
    def setup_class(self):
        normal_path = normal_dir + "m_file3"
        overlay_path = overlay_dir + "m_file3"
        self.source_file = source_file
        self.normal_file = open(normal_path, "rb+")
        self.overlay_file = open(overlay_path, "rb+")
        self.normal_file.seek(0)
        self.data = self.normal_file.read()
    
    def teardown_class(self):
        self.normal_file.truncate(0)
        self.normal_file.seek(0)
        self.normal_file.write(self.data)
        self.normal_file.close()
        self.overlay_file.close()
    
    # read_test
    def test_1(self):
        do_read_cmp(self.normal_file, self.overlay_file, 0, 12780000)
        do_read_cmp(self.normal_file, self.overlay_file, 1000, 12780000)
        do_read_cmp(self.normal_file, self.overlay_file, 21989, 9839200)
        do_read_cmp(self.normal_file, self.overlay_file, 5020, 2000000)
        do_read_cmp(self.normal_file, self.overlay_file, 360000, 8900000)
        do_read_cmp(self.normal_file, self.overlay_file, 1000000,1999999)
        do_read_cmp(self.normal_file, self.overlay_file, 0,3999999)
    
    # read after read
    def test_2(self):
        do_write(self.normal_file, self.overlay_file, 100000, 9870000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 890030, 9990000)
        do_write(self.normal_file, self.overlay_file, 90000, 2000000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 167, 2298357)
        do_write(self.normal_file, self.overlay_file, 115, 30000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 627, 32993)
        do_write(self.normal_file, self.overlay_file, 215000, 1987300, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file,2000, 2000000)
        do_write(self.normal_file, self.overlay_file, 768, 10002390, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 990, 10000000)
        do_write(self.normal_file, self.overlay_file, 20, 14000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 14020)
        do_write(self.normal_file, self.overlay_file, 5020, 20000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 40020)
        do_write(self.normal_file, self.overlay_file, 360, 8900, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 9260)

class TestL:
    def setup_class(self):
        normal_path = normal_dir + "l_file"
        overlay_path = overlay_dir + "l_file"
        self.source_file = source_file
        self.normal_file = open(normal_path, "rb+")
        self.overlay_file = open(overlay_path, "rb+")
        self.normal_file.seek(0)
        self.data = self.normal_file.read()
    
    def teardown_class(self):
        self.normal_file.truncate(0)
        self.normal_file.seek(0)
        self.normal_file.write(self.data)
        self.normal_file.close()
        self.overlay_file.close()
    
    # read_test
    def test_1(self):
        do_read_cmp(self.normal_file, self.overlay_file, 0, 12780000)
        do_read_cmp(self.normal_file, self.overlay_file, 1000, 12780000)
        do_read_cmp(self.normal_file, self.overlay_file, 21989, 9839200)
        do_read_cmp(self.normal_file, self.overlay_file, 5020, 2000000)
        do_read_cmp(self.normal_file, self.overlay_file, 360000, 8900000)
        do_read_cmp(self.normal_file, self.overlay_file, 1000000,1999999)
        do_read_cmp(self.normal_file, self.overlay_file, 0,3999999)
    
    # read after read
    def test_2(self):
        do_write(self.normal_file, self.overlay_file, 100000, 9870000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 890030, 9990000)
        do_write(self.normal_file, self.overlay_file, 90000, 2000000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 167, 2298357)
        do_write(self.normal_file, self.overlay_file, 115, 30000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 627, 32993)
        do_write(self.normal_file, self.overlay_file, 215000, 1987300, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file,2000, 2000000)
        do_write(self.normal_file, self.overlay_file, 768, 10002390, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 990, 10000000)
        do_write(self.normal_file, self.overlay_file, 20, 14000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 14020)
        do_write(self.normal_file, self.overlay_file, 5020, 20000, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 40020)
        do_write(self.normal_file, self.overlay_file, 360, 8900, self.source_file)
        do_read_cmp(self.normal_file, self.overlay_file, 0, 9260)
if __name__ == '__main__':
    pytest.main()

        
