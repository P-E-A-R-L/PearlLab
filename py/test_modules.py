import numbers

from jinja2.nodes import Test
from tensorflow.python.data.ops.optional_ops import Optional

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
print(test.__module__  + "." + test.__qualname__)
print(Param.__module__ + "." + Param.__qualname__)

from pearl.provided.AssaultEnv import AssaultEnvShapMask
from pearl.mask import Mask

print(issubclass(AssaultEnvShapMask, Mask))

print(issubclass(int, numbers.Number))

print(issubclass(numbers.Number, int))


class silly_stuff:
    def __init__(self, x: int, y = 0):
        self.x = x
        self.y = y
        print("Hello from silly_stuff")


def func(x: int, y):
    """
    A simple function that does nothing.

    Args:
        x: An integer parameter.
        y: An optional string parameter with a default value.
    """
    pass

print(func.__code__.co_varnames)

def func_add(x: int, y: int) -> int:
    """
    Adds two integers together.

    Args:
        x: The first integer.
        y: The second integer.

    Returns:
        The sum of x and y.
    """
    return x + y

from builtins import object
from typing import Optional
print(isinstance(1, object().__class__))


def preprocess(obs):
    return obs

from inspect import isfunction

k = preprocess
print(isinstance(preprocess, object))

print(k.__class__)

print(issubclass(str, Optional[str]))