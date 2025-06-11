from annotations import Param


class test:
    integer: Param(int) = 45
    ranged_integer: Param(int, range_start=0, range_end=100) = 50
    floating: Param(float) = 1.4
    ranged_floating: Param(float, range_start=0.0, range_end=1.0) = 0.5
    string: Param(str) = "Hello world"
    string_file: Param(str, isFilePath=True, editable=False, default="C:/path/to/file.txt") = "C:/path/to/file.txt"
    mode: Param(int, choices=[0, 1, 2], default=1) = 1

    def init(self):
        print("That worked :)")
        pass


if __name__ == "__main__":
    t = test()
    print(str(t.__annotations__['string_file']))
    print(t.string_file)