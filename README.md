# mysh

Just a shell.

## Features

### Change Directory

```shell
cd <path>
```

### Executing Binary

mysh will search binary in `/bin` and `/usr/bin`.

```shell
<binary> [...]
```

### Run Background Jobs

```shell
<command> &
```

### Pipe

```shell
<command_1> | <command_2> [ | <command_3> [...] ]
```

### Exit

```shell
exit
```

### Variables

#### Set A Variable
```shell
<var>=<value>
```

#### Expand Variables
```shell
$<var>
```

