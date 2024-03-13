import cppyy
import pytest

def pytest_addoption(parser):
    parser.addoption(
        "--include_files",
        type=str,
        nargs="*",
        default=[],
        help="Extra include files.",
    )
    parser.addoption(
        "--include_dirs",
        type=str,
        nargs="*",
        default=[],
        help="Extra include directories.",
    )

@pytest.fixture(scope="module")
def cppyy_test(request):
    for d in request.config.getoption("--include_dirs"):
        cppyy.add_include_path(d)
    for f in request.config.getoption("--include_files"):
        cppyy.include(f)
    return cppyy.gbl.test

