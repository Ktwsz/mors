from ir_python import Data

def hello_world(data: Data):
    print("hello from python")
    for id in data.ids:
        print(id)


