import functools
from collections.abc import Callable, Hashable, Iterable
from collections import OrderedDict, namedtuple
from functools import wraps


class Cache:
    cache_hits = 0
    cache_misses = 0


def lru_cache(max_items: int) -> Callable:
    """
    Функция создает декоратор, позволяющий кэшировать результаты выполнения обернутой функции по принципу LRU-кэша.
    Размер LRU кэша ограничен количеством max_items. При попытке сохранить новый результат в кэш, в том случае, когда
    размер кэша уже равен max_size, происходит удаление одного из старых элементов, сохраненных в кэше.
    Удаляется тот элемент, к которому обращались давнее всего.
    Также у обернутой функции должен появиться атрибут stats, в котором лежит объект с атрибутами cache_hits и
    cache_misses, подсчитывающие количество успешных и неуспешных использований кэша.
    :param max_items: максимальный размер кэша.
    :return: декоратор, добавляющий LRU-кэширование для обернутой функции.
    """

    orderedDict = OrderedDict()

    def decorator(f):
        f.stats = Cache()

        @wraps(f)
        def wrapper(*args, **kwargs):
            allHash = 0
            for elemArg in args:
                if isinstance(elemArg, Hashable):
                    allHash += hash(elemArg)
                else:
                    allHash += hash(str(elemArg))
            for k, v in kwargs.items():
                allHash += hash(k)
                if isinstance(v, Hashable):
                    allHash += hash(v)
                else:
                    allHash += hash(str(v))

            # Analyzing
            if allHash in orderedDict:
                result = orderedDict[allHash]
                orderedDict.move_to_end(allHash)
                f.stats.cache_hits += 1
            else:
                f.stats.cache_misses += 1
                result = f(*args, **kwargs)
                if len(orderedDict) == max_items:
                    first_key = next(iter(orderedDict))
                    orderedDict.pop(first_key)
                orderedDict[allHash] = result

            return result

        return wrapper

    return decorator
