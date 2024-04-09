from collections.abc import Iterable, Generator

def dfs(tempItem: Iterable):
    if isinstance(tempItem, str):
        yield tempItem
    else:
        for elem in tempItem:
            if isinstance(elem, Iterable):
                if elem:
                    yield from dfs(elem)
            else:
                yield elem


def flatten(iterable: Iterable) -> Generator:
    """
    Генератор flatten принимает итерируемый объект iterable и с помощью обхода в глубину отдает все вложенные объекты.
    Для любых итерируемых вложенных объектов, не являющихся строками, нужно делать рекурсивный заход.
    В результате генератор должен пробегать по всем вложенным объектам на любом уровне вложенности.
    """
    res = dfs(iterable)
    return res