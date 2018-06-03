import module
import numpy as np
from hamcrest import *


def test_create_array():
    c_array = module.array_case.Array(10)
    assert_that(c_array.size, is_(10))

    c_array.set(1, 7)
    assert_that(c_array.get(1), is_(7))

    n_array = np.array(c_array, copy = False)
    assert_that(n_array[1], is_(7))

    n_array[1] = 8
    assert_that(c_array.get(1), is_(8))


def test_create_array__readonly():
    c_array = module.array_case.Array(10)
    assert_that(c_array.size, is_(10))

    c_array.set(1, 7)
    assert_that(c_array.get(1), is_(7))

    n_array = np.array(c_array.to_readonly(), copy = False)
    assert_that(n_array[1], is_(7))

    def assign():
        n_array[1] = 8

    assert_that(calling(assign), raises(Exception))