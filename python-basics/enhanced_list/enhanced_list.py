class EnhancedList(list):
    """
    Улучшенный list.
    Данный класс является наследником класса list и добавляет к нему несколько новых атрибутов.

    - first -- позволяет получать и задавать значение первого элемента списка.
    - last -- позволяет получать и задавать значение последнего элемента списка.
    - size -- позволяет получать и задавать длину списка:
        - если новая длина больше старой, то дополнить список значениями None;
        - если новая длина меньше старой, то удалять значения из конца списка.
    """
#value = ClassName[index] # вызывается метод __getitem__()
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._size = None
        self._first = None
        self._last = None
        self.__update_fls()

    def __update_fls(self):
        self._size = len(self)
        if self._size == 0:
            self._first = None
            self._last = None
        else:
            self._first = self[0]
            self._last = self[self._size - 1]

    def append(self, *args, **kwargs):
        super().append(*args, **kwargs)
        self.__update_fls()

    def extend(self, *args, **kwargs):
        for elem in args:
            super().extend(elem)
        for elem in kwargs:
            super().extend(elem)
        self.__update_fls()

    def pop(self, *args, **kwargs):
        super().pop(*args, **kwargs)
        self.__update_fls()

    def remove(self, *args, **kwargs):
        super().remove(*args, **kwargs)
        self.__update_fls()

    def __getitem__(self, index):
        return super().__getitem__(index)

    def __setitem__(self, index, value):
        super().__setitem__(index, value)
        self.__update_fls()

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, nsize):
        while self._size < nsize:
            self.append(None)
        while self._size > nsize:
            self.pop(self.size - 1)
        self._size = nsize

    @property
    def first(self):
        return self._first

    @first.setter
    def first(self, val):
        self._first = val
        self[0] = val

    @property
    def last(self):
        return self._last

    @last.setter
    def last(self, val):
        self._last = val
        self[self._size - 1] = val
