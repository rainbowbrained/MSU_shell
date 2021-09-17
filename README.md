# MSU_shell

This project implements a subset of the SHELL command language.

APPLICATION MANUAL
=====================

  1. Call ./shell
  2. Print expressions and press enter to start execution.
  3. Press ctrl+D or ctrl+C for immediate terminating (in this case zombie processes 
  might appear).
  
### Handled cases:
  0. Any number of spaces between '<', '>', ';' and other signs. Any number of spaces
  between programms' arguements and redirection signs.
  1. Basic conveyor pr1 | pr2 | ... | prN.
  2. Expression with brackets (wich may contain any kind of embedded brackets).
  3. Execution based on the result of previous programm: pr1 && pr2, pr1 || pr2.
  4. Expressions with programms, waiting for the termination of previous ones: pr1 ; pr2.
  5. Any number of redirection: pr1 > file1 < file2 >> file3 < file4 >>file5 > ...
  6. Execution in a background mode: ... pr&.
  7. Reading an information from any file: ./My_Shell 'filename'. Tests are provided in
  the file 'test.txt'.
  NOTE: if there is no file with such name in the directory/the programm is unable to 
  open it, then My_Shell will get information from the input stream. If there is no 2nd
  arguement, the programm also will get information from the input stream. 
  8. Reading long strings (less then INT_MAX, we try to allocate memory dinamically as
  less as possible to prevent memory leaks due to creating child processes).

THE CONCEPT 
=====================

  The programm consists of My_Shell.c and utilities.c (library). The main programm 
My_Shell.c receives data and calls my_shell_exec from the library. 

  The concept of My_Shell is based on the concept of a simple calculator: the analogy for
numbers is "units", the analogy for operators is "relations" is. Basically, we try to
convert expression with various programms and files into an abstract expression with 
numbers and separators. Therefore, we should define terms "units" and "relations" and 
create an analogy for each relation considering it's priority.

  So, "unit" is an expression, containing 1 programm OR one expression in brackets 
(which may also contain any number of expressions in brackets) to execute, arguements 
and flags (in case it is a programm), any number of filenames, which are redirections of 
input and output. There always must be a name of a programm/expression in brackets in the
beginning of a unit. Optionally, this name may be followed by arguements and flags. 
Finally, everything after the first redirection sign '<' or '>' is considered to be 
instruction for redirections and filenames. Redirections might be:

    - '< filename': The content of a file with the name "filename" will be given to a 
    programm as an input. If the filename does not exist in the directory, an empty file
    is created and passed to the programm as an input.
    
    - '> filename': The result of an executed programm will be written to the filename
    insted of an input stream. If the file with the name "filename" did not exist in the
    directory, it will be created by My_Shell. If the file already exists and is a 
    regular file it will be truncated to length 0.
    
    - '>> filename': The same as the previous redirection, but instead of truncation the
    file in case of it's existence wil be opened in append mode. NOTE: there might be
    spaces between symbols '>'.
    
    NOTE: In the case of multiple input/output redirections, the program writes output 
    in the last given file.

  "Relations" are a kind of separators between units. They specialize the relation 
between two units, therefore we consider them to be binary operators. A relation 
might be:
    
    - 'un1 | un2' - the result if the first unit is given to the second unit even if the
    output of the first unit was redirected to any file.
    
    - 'un1 || un2' - execute the second unit in case of an error in the first unit. The 
    result of the first unit is not passed to the second unit. If the first unit executed
    succsessfully, the second unit (and all the following sequence of units and 
    relations) is not executed.
    
    - 'un1 ; un2' - sequential execution of two units (start the execution of the second
    unit only after finishing the first unit).  The result of the first unit is not 
    passed to the second unit.
    
    - 'un1 && un2' - execute the second unit in case of success in the first unit. The 
    result of the first unit is not passed to the second unit. If the first unit ended
    with an error, the second unit (and all the following sequence of units and 
    relations) is not executed.
    
    - 'un1 &' - the expression is executed in a background mode: My_Shell will continue
    to read another expression and execute it even if the previous execution has not 
    finished.
    
    Priorities and designations:
    - ';' is like a addition + (lower priority).
    - '|','||' and '&&' are like multiplicaton: *, :, / (higher priority).
    
In calculations we use two kinds of structs: Redirect and Unit. There is also an 
array of structs Units.
    Struct Redirect contents all needed information about files for 
one certain unit. This includes:
    
    1. Array of file descriptors of appropriate files;
    
    2. Array of modes of work with appropriate files (consider an input, a place for an
    
    output with truncation/appending);
    
    3. The maximum length of the file's name (1024 bytes by default, may be increased);
    
    4. The maximum amount of the files (1024 by default, may be increased);
    
    5. Current order number of a file.
    
Struct Redirect contents all needed information about files for 
one certain unit. This includes:
    
    1. Struct Redirect with all information about redirection;
    
    2. The name of a file to be executed with a string with arguements;
    
    3. The max length of the string;
        
There is also an array of string for units with brackets, which has a format: 
        (...) *redirections*
    The expression in brackets may contain various different expressions. Redirections
are the same as for basic units.
    
The programm gets expressions line-wisely, then passes each line to the my_shell_exec
function. By default, the input and output streams are considered to be stdin and stdout 
(which may change as the programm runs). Also by default it waits for all processes to 
terminate (if there is no '&' in the end of line). This function fills appropriate tables
and creates an abstract expression with numbers and separators. Each number represents 
the order number of unit in a table of units (if before the number stands 'f', it 
represents the order number of unit in a table of units with brackets). Examples:
    
    1. pr1                                          ->   0
    
       pr1&                                         ->   0&
    
    2. pr1 | pr2 || pr3 && pr4                      ->   0|1=2/3
    
    3. pr1 | (pr2 ; pr3)                            ->   0|f0
    
    4. pr1 ; (pr2)|(pr3|(pr4 ; pr5|(pr6))) && pr7   ->   0;f0|f1/1
    
Then the function passes abstract expression to comveyour, which binds all the 
information together, creates pipes, redirects inputs and outputs (if there are any 
redirection files), starts new processes and coordinates their execution.
    
    - If a new unit is a unit with brackets, the function analizes given redirection 
    files, and then a new process is created, which calls my_exec_function. Conveior
    gives appropeiate file descriptors as input for my_exec_function.
    
    - If a unit has multiple output redirection files, it writes to other files from the
    last one, which is associated with stdout (see NOTES in the description of
    redirections).
    
 Special case: (pr1) && pr2 
    
    - Main process creates child processes for (pr1). 
    
    - Child (pr1) creates a grandchild process for pr1. Then the child process wait for
    the terminationg of grandchild (in case of an error WEXITSTATUS = 100). Then the
    child process terminates with the same WEXITSTATUS, as it's last grandchild.
    
    - Main process gets WEXITSTATUS of (pr1). If it equals 100, it creates a child 
    process for pr2. 
    
    - Generalizing: in expressions with forms (execution after brackets depends on the
    result of programms in brackets)
        (pr1 ... prN) || 1pr ... Mpr      or      (pr1 ... prN) && 1pr ... Mpr
    Child process passes the exit status of the LAST grandchild process to the main 
    process. Then the main process continues work depending on the given exit status.


