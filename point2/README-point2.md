# Lab 3.2 — Файлы и файловые системы
## Работа с файлами, каталогами, символьными и жёсткими ссылками

# 1. Идея программы

Используется **один бинарник**, а конкретное действие выбирается по имени, под которым этот бинарник запущен.

После компиляции получается файл:

```bash
fs_tool
```

На него создаются **жёсткие ссылки** с нужными именами:

```bash
ln fs_tool mkd
ln fs_tool lsd
ln fs_tool rmd
ln fs_tool mkf
ln fs_tool catf
ln fs_tool rmf
ln fs_tool mksym
ln fs_tool catsym
ln fs_tool catbylink
ln fs_tool rmsym
ln fs_tool mkhard
ln fs_tool rmhard
ln fs_tool statf
ln fs_tool chmodf
```

Дальше программа смотрит на `argv[0]` и понимает, какую функцию нужно выполнить.

---

# 2. Сборка и запуск

## Компиляция

```bash
gcc -O2 -Wall -Wextra -Werror -std=c11 lab3_2.c -o fs_tool
```

Флаги:

| флаг | значение |
|------|----------|
| `-O2` | оптимизация |
| `-Wall` | основные предупреждения |
| `-Wextra` | дополнительные предупреждения |
| `-Werror` | warnings превращаются в errors |
| `-std=c11` | стандарт языка C |

---

## Создание имён-команд

```bash
ln fs_tool mkd
ln fs_tool lsd
ln fs_tool rmd
ln fs_tool mkf
ln fs_tool catf
ln fs_tool rmf
ln fs_tool mksym
ln fs_tool catsym
ln fs_tool catbylink
ln fs_tool rmsym
ln fs_tool mkhard
ln fs_tool rmhard
ln fs_tool statf
ln fs_tool chmodf
```

Проверка:

```bash
ls -li fs_tool mkd lsd rmd mkf catf rmf mksym catsym catbylink rmsym mkhard rmhard statf chmodf
```

У всех файлов должен быть **один и тот же inode number**.

---

# 3. Краткая карта действий

| имя запуска | действие |
|-------------|----------|
| `mkd` | создать каталог |
| `lsd` | вывести содержимое каталога |
| `rmd` | удалить каталог |
| `mkf` | создать файл |
| `catf` | вывести содержимое файла |
| `rmf` | удалить файл |
| `mksym` | создать symlink |
| `catsym` | вывести содержимое symlink |
| `catbylink` | вывести содержимое файла через symlink |
| `rmsym` | удалить symlink |
| `mkhard` | создать hard link |
| `rmhard` | удалить hard link |
| `statf` | вывести права и `nlink` |
| `chmodf` | изменить права |

---

# 4. Базовая модель VFS: inode, dentry, file

## 4.1 `inode`

`inode` — это **сам объект файловой системы**.

Он хранит:

- тип файла
- права доступа
- владельца
- размер
- timestamps
- число жёстких ссылок
- ссылки на блоки данных файла

Упрощённо:

```c
struct inode {
    umode_t i_mode;
    loff_t  i_size;
    nlink_t i_nlink;
    struct super_block *i_sb;
}
```

### Главное
`inode` **не хранит имя файла**.

---

## 4.2 `dentry`

`dentry` = **directory entry**.

Это объект, который связывает:

```text
имя -> inode
```

Упрощённо:

```c
struct dentry {
    struct inode  *d_inode;
    struct dentry *d_parent;
    struct qstr    d_name;
}
```

### Главное
`dentry` отвечает за **имя файла в каталоге**.

---

## 4.3 `struct file`

`struct file` — это **открытый экземпляр файла**.

Он создаётся при `open()` и хранит:

- inode файла
- текущую позицию чтения/записи
- флаги открытия

Упрощённо:

```c
struct file {
    struct inode *f_inode;
    loff_t        f_pos;
    fmode_t       f_mode;
    struct file_operations *f_op;
}
```

### Главное
`inode` и `file` — разные вещи:

- `inode` — сам объект на FS
- `struct file` — результат конкретного `open()`

---

## 4.4 Общая цепочка

```text
process
  ↓
task_struct
  ↓
files_struct
  ↓
fdtable
  ↓
struct file
  ↓
dentry
  ↓
inode
  ↓
filesystem
```

### Что означает эта цепочка

- у процесса есть `task_struct`
- в нём есть `files_struct`
- там находится `fdtable`
- `fd` — это индекс в `fdtable`
- `fdtable[fd]` указывает на `struct file`
- `struct file` связан с `dentry` и `inode`
- `inode` принадлежит конкретной файловой системе

---

# 5. Что такое `super_block`

`super_block` — это структура ядра, которая описывает **всю файловую систему**, а не отдельный файл.

В ней хранится:

- тип файловой системы
- размер блока
- корневой каталог
- таблицы операций файловой системы

Упрощённо:

```c
struct super_block {
    struct file_system_type *s_type;
    struct super_operations *s_op;
    struct dentry *s_root;
    unsigned long s_blocksize;
}
```

Каждый `inode` содержит указатель:

```c
inode->i_sb
```

То есть файл знает, в какой файловой системе он лежит.

---

# 6. Что такое path lookup

**Path lookup** — это алгоритм ядра Linux, который преобразует строку пути в `dentry` и `inode`.

Пример:

```text
./a/b/file.txt
```

Ядро разбивает путь на компоненты:

```text
.
a
b
file.txt
```

и последовательно ищет их в каталогах.

Используются:

- `dentry cache`
- `inode`
- методы конкретной файловой системы (`lookup`)

---

## Ускорение: dentry cache

Если имя уже недавно искали, ядро может найти его в **dentry cache** и не читать каталог с диска.

---

# 7. Каталог — это тоже файл

Каталог — это inode типа:

```text
S_IFDIR
```

Его данные содержат записи:

```text
name -> inode
```

То есть каталог — это таблица соответствия **имя → inode**.

Пример:

```text
"."      -> inode текущего каталога
".."     -> inode родителя
"file"   -> inode файла
"subdir" -> inode подкаталога
```

---

## Почему у нового каталога `nlink = 2`

У нового каталога есть две directory entries, указывающие на его inode:

1. имя каталога в родительском каталоге
2. запись `.` внутри самого каталога

Запись `..` указывает на inode родителя, а не на inode текущего каталога.

---

# 8. Hard link и symlink

## 8.1 Hard link

Hard link — это **ещё одно имя для того же inode**.

Пример:

```bash
ln file.txt file2.txt
```

Получается:

```text
file.txt  -> inode 100
file2.txt -> inode 100
```

У inode увеличивается:

```text
i_nlink++
```

### Главное
Hard link не создаёт новый inode.

---

## 8.2 Symlink

Symlink — это **отдельный inode типа `S_IFLNK`**, внутри которого хранится строка пути.

Пример:

```bash
ln -s file.txt link.txt
```

Получается:

```text
link.txt -> inode 200
```

А внутри inode 200 лежит строка:

```text
"file.txt"
```

### Главное
Symlink создаёт новый inode.

---

## 8.3 Разница

| свойство | hard link | symlink |
|----------|-----------|---------|
| новый inode | нет | да |
| новый dentry | да | да |
| указывает на | тот же inode | путь-строку |
| `i_nlink` цели | увеличивается | не увеличивается |
| если удалить исходное имя | файл остаётся | symlink может стать битым |
| можно между разными FS | нет | да |

---

# 9. Почему hard link на каталог запрещён

Hard link на каталог может создать **циклы** в дереве каталогов.

Пример опасной ситуации:

```text
a/
└── b/
    └── link -> a
```

Тогда получится цикл:

```text
a/b/link/b/link/b/link/...
```

Это ломает:

- path lookup
- рекурсивный обход
- удаление каталогов

Поэтому `link()` для каталогов обычно запрещён.

---

# 10. Почему `unlink()` нельзя для каталогов

`unlink()` удаляет directory entry и уменьшает `i_nlink`.

Для обычного файла это безопасно.  
Для каталога это может оставить **осиротевший каталог**, в котором ещё есть записи.

Поэтому для каталогов используется `rmdir()`, который удаляет **только пустой каталог**.

---

# 11. Почему `rm -rf` всё же удаляет каталоги

`rm -rf` не вызывает `unlink()` для каталогов напрямую.

Он делает:

1. `readdir()` — читает содержимое каталога
2. `unlink()` — удаляет файлы и symlink
3. рекурсивно обходит подкаталоги
4. когда каталог становится пустым — вызывает `rmdir()`

То есть удаление идёт **снизу вверх**.

---

# 12. Используемые системные вызовы

| libc-функция | syscall | сигнатура | что делает |
|--------------|--------|-----------|------------|
| `open()` | `openat` | `int openat(int dirfd, const char *pathname, int flags, mode_t mode);` | открывает файл, создаёт `struct file`, возвращает файловый дескриптор `fd` |
| `opendir()` | `openat` | `int openat(int dirfd, const char *pathname, int flags, mode_t mode);` | открывает каталог как файл (обычно с `O_DIRECTORY`) |
| `readdir()` | `getdents64` | `ssize_t getdents64(unsigned int fd, struct linux_dirent64 *dirp, size_t count);` | читает записи каталога (`directory entries`) |
| `mkdir()` | `mkdirat` | `int mkdirat(int dirfd, const char *pathname, mode_t mode);` | создаёт новый каталог |
| `rmdir()` | `rmdir` | `int rmdir(const char *pathname);` | удаляет пустой каталог |
| `unlink()` | `unlinkat` / `unlink` | `int unlinkat(int dirfd, const char *pathname, int flags);` | удаляет directory entry и уменьшает `inode->i_nlink` |
| `lstat()` | `newfstatat` | `int newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags);` | получает метаданные файла без разыменования symlink |
| `readlink()` | `readlinkat` / `readlink` | `ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);` | читает строку пути из символьной ссылки |
| `symlink()` | `symlinkat` / `symlink` | `int symlinkat(const char *target, int newdirfd, const char *linkpath);` | создаёт символьную ссылку |
| `link()` | `linkat` / `link` | `int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);` | создаёт жёсткую ссылку на тот же inode |
| `chmod()` | `chmod` / `fchmodat` | `int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);` | изменяет права доступа (`inode->i_mode`) |
| `read()` | `read` | `ssize_t read(int fd, void *buf, size_t count);` | читает данные из файла по `fd` |
| `write()` | `write` | `ssize_t write(int fd, const void *buf, size_t count);` | записывает данные в файл по `fd` |
| `close()` | `close` | `int close(int fd);` | закрывает файловый дескриптор |

---

# 13. Как работают отдельные операции

## 13.1 Создать каталог — `mkd`

Команда:

```bash
./mkd dir1
```

Внутри:

```c
mkdir(path, 0755)
```

### Что делает ядро

1. path lookup родительского каталога
2. создаёт inode типа `S_IFDIR`
3. добавляет запись в родительский каталог:

```text
name -> inode
```

4. создаёт внутри нового каталога записи:

```text
.  -> self
.. -> parent
```

---

## 13.2 Вывести содержимое каталога — `lsd`

Команда:

```bash
./lsd dir1
```

Внутри:

```c
opendir()
readdir()
closedir()
```

### Что делает ядро

- `opendir()` открывает каталог как файл
- `readdir()` внутри использует `getdents64`
- ядро читает directory entries
- libc преобразует их в `struct dirent`

---

## 13.3 Удалить каталог — `rmd`

Команда:

```bash
./rmd dir1
```

Внутри:

```c
rmdir(path)
```

### Что делает ядро

1. path lookup
2. проверка, что inode — каталог
3. проверка, что каталог пуст
4. удаление записи из родительского каталога
5. уменьшение link count
6. освобождение inode

---

## 13.4 Создать файл — `mkf`

Команда:

```bash
./mkf file.txt
```

Внутри:

```c
open(path, O_WRONLY | O_CREAT | O_EXCL, 0644)
```

### Что значат флаги

- `O_WRONLY` — открыть только на запись
- `O_CREAT` — создать файл, если его нет
- `O_EXCL` — если уже существует, вернуть ошибку

### Что делает ядро

1. path lookup родителя
2. создаёт inode типа `S_IFREG`
3. создаёт dentry:

```text
file.txt -> inode
```

4. создаёт `struct file`
5. возвращает fd

---

## 13.5 Вывести содержимое файла — `catf`

Команда:

```bash
./catf file.txt
```

Внутри:

```c
open(path, O_RDONLY)
read()
write(STDOUT_FILENO, ...)
close()
```

### Что делает ядро

`open()` создаёт `struct file`, затем `read()` по fd идёт к inode и читает данные.

---

## 13.6 Удалить файл — `rmf`

Команда:

```bash
./rmf file.txt
```

Внутри:

```c
unlink(path)
```

### Что делает ядро

1. path lookup
2. удаляет directory entry
3. уменьшает `inode->i_nlink`
4. если `i_nlink == 0` и файл не открыт — удаляет inode

### Важно
`unlink()` удаляет **имя**, а не обязательно сразу данные.

---

## 13.7 Создать symlink — `mksym`

Команда:

```bash
./mksym target.txt sym1
```

Внутри:

```c
symlink(target, linkpath)
```

### Что делает ядро

1. создаёт inode типа `S_IFLNK`
2. записывает внутрь него строку `"target.txt"`
3. добавляет dentry:

```text
sym1 -> inode_symlink
```

---

## 13.8 Вывести содержимое symlink — `catsym`

Команда:

```bash
./catsym sym1
```

Внутри:

```c
readlink(path, buf, ...)
```

### Что делает ядро

Читает **строку, лежащую внутри inode symlink**.

### Важно
`readlink()` **не следует по ссылке**.

---

## 13.9 Вывести содержимое файла через symlink — `catbylink`

Команда:

```bash
./catbylink sym1
```

Внутри:

```c
open(path_to_symlink, O_RDONLY)
read()
```

### Что делает ядро

1. path lookup находит inode symlink
2. читает из него путь цели
3. продолжает lookup уже для цели
4. создаёт `struct file` для целевого inode
5. `read()` читает содержимое уже целевого файла

---

## 13.10 Удалить symlink — `rmsym`

Команда:

```bash
./rmsym sym1
```

Внутри:

```c
unlink(path)
```

### Что делает ядро

Удаляет **сам symlink**, то есть dentry и inode типа `S_IFLNK`.

### Важно
Файл-цель при этом не трогается.

---

## 13.11 Создать hard link — `mkhard`

Команда:

```bash
./mkhard target.txt hard1
```

Внутри:

```c
link(oldpath, newpath)
```

### Что делает ядро

1. path lookup `oldpath`
2. получает inode старого файла
3. path lookup родителя `newpath`
4. создаёт новый dentry:

```text
hard1 -> inode_old
```

5. увеличивает:

```text
inode_old->i_nlink++
```

### Главное
Новый inode не создаётся.

---

## 13.12 Удалить hard link — `rmhard`

Команда:

```bash
./rmhard hard1
```

Внутри:

```c
unlink(path)
```

### Что делает ядро

Удаляет одно из имён inode и уменьшает `i_nlink`.

Если остаются другие hard links, файл продолжает существовать.

---

## 13.13 Вывести права и число hard links — `statf`

Команда:

```bash
./statf target.txt
```

Внутри:

```c
lstat(path, &st)
```

### Что такое `st_mode`

Биты:

- тип файла
- права доступа

### Что такое `st_nlink`

Количество жёстких ссылок на inode.

---

## 13.14 Изменить права — `chmodf`

Команда:

```bash
./chmodf target.txt 0600
```

Внутри:

```c
chmod(path, mode)
```

### Что делает ядро

Изменяет биты прав в inode:

```text
inode->i_mode
```

### Важно
Если у файла есть hard links, права изменятся для всех имён этого inode.

---

# 14. `stat`, `lstat`, `fstat`

| функция | что принимает | что показывает |
|---------|---------------|----------------|
| `stat(path)` | путь | свойства цели, если путь — symlink |
| `lstat(path)` | путь | свойства самого symlink |
| `fstat(fd)` | fd | свойства уже открытого объекта |

---

# 15. Как работает `open()`

Пользовательский вызов:

```c
open(path, flags)
```

glibc обычно вызывает syscall:

```text
openat()
```

### Что происходит внутри ядра

1. CPU выполняет инструкцию `syscall`
2. управление переходит в `__x64_sys_openat`
3. выполняется path lookup
4. ядро ищет имя в `dentry cache`
5. если не найдено — читает каталог с диска
6. находит inode
7. создаёт `struct file`
8. добавляет его в `fdtable`
9. возвращает fd

---

# 16. Как работает `read()`

Последовательность:

```text
read()
 ↓
sys_read
 ↓
vfs_read
 ↓
filesystem read
```

Сначала ядро проверяет **page cache**.

---

## Cache hit

Нужная страница файла уже находится в page cache:

```text
page cache -> user buffer
```

Диск не используется.

---

## Cache miss

Нужной страницы нет в page cache:

```text
disk -> DMA -> RAM -> page cache -> user buffer
```

1. ядро формирует запрос к устройству
2. контроллер диска выполняет DMA
3. данные попадают в RAM
4. страница добавляется в page cache
5. данные копируются процессу

---

# 17. Что такое DMA

**DMA (Direct Memory Access)** — это механизм, при котором устройство передаёт данные **напрямую в RAM**, а CPU только настраивает передачу и получает interrupt по завершении.

Без DMA CPU пришлось бы копировать каждый байт сам.

---

# 18. Почему `lseek()` не читает диск

`lseek(fd, offset, SEEK_SET)` изменяет только:

```text
struct file -> f_pos
```

Это изменение текущей позиции чтения/записи в уже открытом файле.

Никакого disk I/O при этом не происходит.

---

# 19. Почему исходное имя файла тоже hard link

Когда файл создаётся впервые, в каталоге появляется запись:

```text
file.txt -> inode 100
```

Это уже одна жёсткая ссылка на inode.

Поэтому у нового файла обычно:

```text
st_nlink = 1
```

После `link()` становится 2, 3 и т.д.

---

# 20. Почему `rm -rf /` не ломает файловую систему

`rm -rf /` не удаляет каталоги напрямую через `unlink()`.

Он:

1. читает содержимое каталогов
2. удаляет файлы через `unlink()`
3. рекурсивно обходит подкаталоги
4. удаляет пустые каталоги через `rmdir()`

То есть структура дерева не нарушается, хотя система может стать неработоспособной из-за удаления критических файлов.

---

# 21. Последовательность проверки всех подпунктов

## 1. Создать каталог

```bash
./mkd dir1
ls -ld dir1
```

## 2. Вывести содержимое каталога

```bash
./lsd .
./lsd dir1
```

## 3. Удалить каталог

```bash
./rmd dir1
```

## 4. Создать файл

```bash
./mkf file1.txt
ls -l file1.txt
```

## 5. Вывести содержимое файла

```bash
printf "hello world
" > file1.txt
./catf file1.txt
```

## 6. Удалить файл

```bash
./rmf file1.txt
```

## 7. Создать symlink

```bash
printf "target content
" > target.txt
./mksym target.txt sym1
ls -l sym1
```

## 8. Вывести содержимое symlink

```bash
./catsym sym1
```

## 9. Вывести содержимое файла через symlink

```bash
./catbylink sym1
```

## 10. Удалить symlink

```bash
./rmsym sym1
```

## 11. Создать hard link

```bash
./mkhard target.txt hard1
ls -li target.txt hard1
```

## 12. Удалить hard link

```bash
./rmhard hard1
```

## 13. Вывести права и `nlink`

```bash
./statf target.txt
./mkhard target.txt hard2
./statf target.txt
```

## 14. Изменить права

```bash
./chmodf target.txt 0600
ls -l target.txt hard2
./statf target.txt
```

---

# 22. Самые частые вопросы на защите

## Что такое file descriptor?

Это индекс в таблице файлов процесса:

```text
fd -> struct file
```

---

## Чем отличается inode от dentry?

- `inode` — сам объект
- `dentry` — имя объекта в каталоге

---

## Чем отличается inode от struct file?

- `inode` — метаданные объекта на FS
- `struct file` — открытый экземпляр объекта

---

## Почему `lstat`, а не `stat`?

Потому что `stat()` следует по symlink, а `lstat()` показывает саму ссылку.

---

## Почему chmod на hard-linked именах меняет права у всех?

Потому что у них один и тот же inode, а права хранятся в inode.

---

## Почему удаление hard link не удаляет файл сразу?

Потому что `unlink()` уменьшает `i_nlink`. Пока остаются другие имена или открытые `struct file`, inode живёт.

---

## Почему symlink можно создать на несуществующий файл?

Потому что symlink хранит просто строку пути. Цель не обязана существовать в момент создания.

---

## Почему `readlink()` и `open()` ведут себя по-разному для symlink?

- `readlink()` читает строку внутри symlink
- `open()` разыменовывает symlink и открывает цель

---

## Почему hard link на каталог запрещён?

Потому что это может создать циклы и сломать дерево каталогов.

---

## Почему `unlink()` нельзя для каталогов?

Потому что каталог может содержать другие записи; для него нужен `rmdir()`, который требует пустой каталог.
