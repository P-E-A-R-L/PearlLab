from annotations import Param


class test:
    integer: Param(int) = 45
    ranged_integer: Param(int, range_start=0, range_end=100) = 50
    floating: Param(float) = 1.4
    ranged_floating: Param(float, range_start=0.0, range_end=1.0) = 0.5
    string: Param(str) = "Hello world"
    string_file: Param(str, isFilePath=True, editable=False, default="C:/path/to/file.txt") = "C:/path/to/file.txt"
    mode: Param(int, choices=[0, 1, 2], default=1) = 1

    def __init__(self, x: int , y = None):
        pass


print(Param.__init__)