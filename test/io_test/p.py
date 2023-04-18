import time

start = time.time()
f = open("./merged/file_2g","a+")
end = time.time()
print("open time: ", end-start)
#fio fio.conf --output-format=json --output=fio_res.json