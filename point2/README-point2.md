# Lab 1

## Программа в файле `hello.c`, библиотека в файлах `static_lib`

------------------------------------------------------------------------

## 1. Собираем библиотеку

``` bash
gcc -c static_lib.c -o static_lib.o
```

gcc сейчас не создал исполняемый файл, он сделал relocatable object, внутри
которого код функции hello_from_static_lib лежит в секции .text, символ
hello_from_static_lib определен (T), puts также U.

```bash
ar rcs libstatic.a static_lib.o
```

libstatic.a - архив(контейнер с несколькими файлами, упакованными для линковки),
внутри которого лежит static_lib.o

### Для проверки:
```bash
ar t libstatic.a
```

Вывод:
    
    static_lib.o

------------------------------------------------------------------------

## 2. Линкуем hello со статической библиотекой

``` bash
gcc hello.c ./libstatic.a -o hello
```

### Проверка динамической таблицы

```bash
nm -D hello | grep hello_from_static_lib
```
Вывод:

    Пусто

### Проверка всех символов

```bash
nm hello | grep hello_from_static_lib
```
Вывод:

    000000000000116c T hello_from_static_lib

Значит символ определен, эта функция лежит в коде и встроена в этот файл,
функция не static(глобально экспортируема), т.к. T большая

------------------------------------------------------------------------

## 3. Собираем динамическую библиотеку

### Компилируем объектник
``` bash
gcc -fPIC -c dynamic_lib.c -o dynamic_lib.o
```

-fPIC - генерирует position-independent code - нужно для .so, потому что динамическая
библиотека может быть загружена по разным адресам, код должен работать корректно независимо от адреса

```bash
nm dynamic_lib.o | grep -E "hello_from_dynamic_lib|puts"
```

Вывод:

    0000000000000000 T hello_from_dynamic_lib
    U puts

gcc сейчас создал опять relocatable object. Внутри dynamic_lib.o код функции
hello_from_dynamic_lib лежит в секции .text, этот символ определен(T), а puts U,
потому что реализация puts находится в другой динамической библиотеке.

------------------------------------------------------------------------

### 4. Линкуем shared library .so
```bash
gcc -shared -o libdynamic.so dynamic_lib.o
```

libdynamic.so - ELF shared object, который будет подгружаться во время запуска программы
динамическим загрузчиком

Проверяем экспорт функции:
```bash
nm -D libdynamic.so | grep hello_from_dynamic_lib
```

Вывод:

    0000000000001199 T hello_from_dynamic_lib

Это означает что библиотека экспортирует эту функцию для использования другими программами

------------------------------------------------------------------------

## 5. Линкуем hello с динамической библиотекой

### Собираем исполняемый файл
```bash
gcc hello.c ./libdynamic.so -o hello
```

На этом шаге линкер добавляет зависимость `NEEDED libdynamic.so`, оставляет символ hello_from_dynamic_lib
как неопределенный.

```bash
nm -D hello | grep hello_from_dynamic_lib
```

Вывод:

    U hello_from_dynamic_lib

### Смотрим зависимости hello

```bash
ldd ./hello
```

Вывод:

    linux-vdso.so.1 (0x00007ffdc3dfd000)
    ./libdynamic.so (0x00007870fd1e8000)
    libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007870fce00000)
    /lib64/ld-linux-x86-64.so.2 (0x00007870fd1f4000)

видим нашу библиотеку

------------------------------------------------------------------------

## 6. Запускаем hello

U символ разрешается аналогично puts в первом пункте лабораторной, только библиотека отличается

hello содержит запись NEEDED libdynamic.so, при запуске ld-linux загружает .so, ищет символ 
hello_from_dynamic_lib в .dynsym библиотеки, заполняет GOT, вызов идет через hello_from_dynamic_lib@plt