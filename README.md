# Custom Shell Implementation in C

A fully-featured custom shell implementation written in C using POSIX library standards. This shell provides a comprehensive command-line interface with support for built-in commands, external program execution, I/O redirection, piping, job control, and signal handling.

## 🚀 Features

### Built-in Commands (Intrinsics)
- **`hop`** - Navigate directories with support for `~`, `.`, `..`, `-` (previous directory)
- **`reveal`** - List directory contents with flags:
  - `-a`: Show hidden files
  - `-l`: Long format (line-by-line listing)
- **`log`** - Command history management:
  - `log`: Display command history (last 15 commands)
  - `log purge`: Clear command history
  - `log execute <index>`: Re-execute a command from history
- **`activities`** - Display all background processes sorted by command name
- **`ping <pid> <signal>`** - Send signals to processes
- **`fg [job_id]`** - Bring background job to foreground
- **`bg [job_id]`** - Resume stopped background job

### Advanced Features
- **Process Execution**: Execute external commands and system programs
- **I/O Redirection**: 
  - Input redirection: `<`
  - Output redirection: `>` (overwrite), `>>` (append)
- **Piping**: Chain commands with `|`
- **Job Control**:
  - Background process execution with `&`
  - Foreground/background job management
  - Track process states (Running/Stopped)
- **Signal Handling**: Proper handling of `Ctrl-C` (SIGINT) and `Ctrl-Z` (SIGTSTP)
- **Command Chaining**: Execute multiple commands sequentially with `;`
- **Command History**: Persistent history stored in `history.txt` (max 15 commands)

## 📁 Project Structure

```
Shell_Commands/
├── Makefile              # Build configuration
├── README.md             # Project documentation
├── include/              # Header files
│   ├── execution.h       # Process execution declarations
│   ├── fileredirection.h # I/O redirection declarations
│   ├── intrinsics.h      # Built-in commands declarations
│   ├── parse_input.h     # Input parsing declarations
│   └── signals.h         # Signal handling declarations
└── src/                  # Source files
    ├── execution.c       # Process execution & job control
    ├── fileredirection.c # I/O redirection & piping
    ├── intrinsics.c      # Built-in command implementations
    ├── parse_input.c     # Command line parsing
    ├── shell.c           # Main shell loop
    └── signals.c         # Signal handlers
```

## 🛠️ Building the Project

### Prerequisites
- GCC compiler (C99 standard or later)
- POSIX-compliant system (Linux, macOS, WSL)
- Make utility

### Compilation

```bash
# Build the shell
make

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild
```

The build process will create an executable named `shell.out`.

### Compiler Flags
- `-std=c99`: C99 standard
- `-D_POSIX_C_SOURCE=200809L`: POSIX 2008 compliance
- `-D_XOPEN_SOURCE=700`: X/Open 7 compliance
- `-Wall -Wextra -Werror`: Enable all warnings and treat them as errors

## 🚦 Usage

### Starting the Shell

```bash
./shell.out
```

The shell will display a prompt in the format:
```
<username@hostname:current_directory>
```

### Command Examples

#### Navigation (hop)
```bash
hop ~                    # Go to home directory
hop /usr/local/bin       # Go to absolute path
hop ..                   # Go to parent directory
hop -                    # Go to previous directory
hop dir1 dir2            # Navigate through multiple directories
```

#### Directory Listing (reveal)
```bash
reveal                   # List current directory
reveal -a                # Show hidden files
reveal -l                # Long format listing
reveal -al path/to/dir   # Combine flags and specify path
```

#### Command History (log)
```bash
log                      # Show command history
log execute 3            # Execute 3rd command from history
log purge                # Clear command history
```

#### Process Management
```bash
sleep 100 &              # Run in background
activities               # List all background jobs
ping 1234 9              # Send SIGKILL to process 1234
fg 1                     # Bring job 1 to foreground
bg 1                     # Resume job 1 in background
```

#### I/O Redirection
```bash
cat < input.txt          # Redirect input
echo "Hello" > out.txt   # Redirect output (overwrite)
echo "World" >> out.txt  # Redirect output (append)
cat < in.txt > out.txt   # Both input and output redirection
```

#### Piping
```bash
ls -l | grep ".c"        # Pipe ls output to grep
cat file.txt | wc -l     # Count lines in file
```

#### Command Chaining
```bash
echo "First" ; echo "Second"    # Sequential execution
ls -l ; pwd ; date               # Multiple commands
```

## 🔧 Technical Details

### Key Implementation Features

1. **Custom Input Parser**: Tokenizes and validates command syntax
2. **Job Control**: Maintains job table with process states (Running/Stopped/Completed)
3. **Signal Management**: 
   - Shell ignores `SIGINT` and `SIGTSTP`
   - Child processes receive default signal handlers
   - Foreground processes properly handle interrupts
4. **Process Groups**: Proper terminal control and process group management
5. **Memory Management**: Dynamic memory allocation with proper cleanup
6. **Error Handling**: Comprehensive error checking and reporting

### Limitations
- Maximum command input length: 1024 characters
- Maximum arguments per command: 64
- Maximum concurrent background jobs: 64
- Command history size: 15 entries

## 📝 Files Generated at Runtime

- `history.txt`: Stores command history persistently across shell sessions
- `shell.out`: Compiled executable

## 🐛 Error Handling

The shell provides clear error messages for:
- Invalid commands
- File not found errors
- Permission denied
- Invalid syntax
- Process execution failures
- I/O redirection errors

## 👨‍💻 Development

### Code Standards
- POSIX C Source 200809L compliance
- X/Open Source 700 compliance
- Strict warning enforcement (`-Wall -Wextra -Werror`)
- Clean, modular architecture

### Testing
Test various scenarios including:
- Simple command execution
- Built-in commands with various flags
- I/O redirection and piping
- Background and foreground job switching
- Signal handling (Ctrl-C, Ctrl-Z)
- Command history operations
- Edge cases and error conditions

## 📄 License

This project is part of an academic assignment and follows the institution's code of conduct.

## 🤝 Contributing

This is an educational project. Contributions should maintain:
- Code quality and readability
- POSIX compliance
- Proper error handling
- Memory safety

## 📚 References

- POSIX.1-2008 Standard
- Advanced Programming in the UNIX Environment (APUE)
- Linux man pages for system calls

---

**Note**: This shell is designed for educational purposes and may not include all features of production shells like bash or zsh.