#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#define N 10
#define ERROR -1
#define METPALKA 0
#define DIDNTMETPALKA 1
#define ENDINPUT 2
#define Syntax_ERROR 3
#define Pipe_ERROR 4
#define End_of_Work 5
#define FIRST 6
#define START 7

typedef struct {
    char* input;
    char* output;
    char* outputcur;
    char* error;
    char** argv;
    int status;
} data;

char* new_arg( int* status, char* end ) {
    int quotes=0, apostrophes=0, i=0, n=1;
    char c;
    char* arg=(char*)calloc( N, sizeof(char) );
    if ( arg == NULL ){
        fprintf( stderr, "%s", "Not Enough Memory\n" );
        *status = ERROR;
        return arg;
    }

    if ( scanf( "%c", &c ) == EOF ){
        *status = End_of_Work;
        return arg;
    }
    while( c == ' ' ) if (scanf( "%c", &c ) == EOF){
            *status = End_of_Work;
            return arg;
        }

    if ( ((c == '>') || (c == '<') || (c == '2')) && ((*status == METPALKA) || (*status == START)) ){
        *status = ERROR;
        *end = c;
        fprintf( stderr, "%s", "Syntax error\n" );
        return arg;
    }

    if( (c == '|') || (c == '\n') ){
        if ( (*status == METPALKA) || (*status == START) ){
            if ( (c == '|') || ( (c == '\n') && (*status != START) ) ) fprintf( stderr, "%s", "Syntax error\n" );
            *status = ERROR;
            *end = c;
            return arg;
        }
        else {
            if ( c == '|' ) *status = METPALKA;
            if ( c == '\n' ) *status = ENDINPUT;
            arg[0] = '\0';
            *end = c;
            return arg;
        }
    }

    *status = DIDNTMETPALKA;

    while( (c!='\n') && ( (c!='|') || quotes || apostrophes ) && ( (c!=' ') || quotes || apostrophes ) && (c!='>') && (c!='<') ){

        if ( i-2 > N ){
            n++;
            arg = (char*)realloc( arg, n * N * sizeof(char) );
            if ( arg == NULL ){
                arg[i] = '\0';
                fprintf( stderr, "%s", "Not Enough Memory\n" );
                *status = ERROR;
                return arg;
            }
        }
        arg[i] = c;
        i++;
        if ( c == '"' ){
            if ( quotes ) {
                quotes = 0;
                apostrophes = 0;
            }
            else quotes = 1;
            if (apostrophes == 0) i--;
        }
        if ( c == '\'' ){
            if ( apostrophes ) {
                apostrophes = 0;
                quotes = 0;
            }
            else apostrophes = 1;
            if (quotes == 0) i--;
        }
        if (scanf( "%c", &c ) == EOF){
            *status = End_of_Work;
            return arg;
        }
    }

    if ( ((c == '<') || (c == '>')) && (i == 0) ){
        arg[i] = c;
        arg[i+1] = '\0';
        *end = ' ';
    }
    else {
        *end = c;
        arg[i] = '\0';
    }

    if ( apostrophes || quotes ){
        fprintf( stderr, "%s", "Syntax error\n" );
        *status = ERROR;
        return arg;
    }

    if(c == '\n') *status = ENDINPUT;
    if(c == '|') *status = METPALKA;

    return arg;
}

char** new_argv( int* status, char* pend ){
    int argc=0, n=1;
    char end = '.';
    char**argv = (char**)malloc( sizeof(char*) * N );
    if ( argv == NULL ){
        fprintf( stderr, "%s", "Not Enough Memory\n" );
        *status = ENDINPUT;
        *pend = end;
        return argv;
    }

    argv[argc] = new_arg( status, &end );
    argc++;

    while( (*status != METPALKA) && (*status != ENDINPUT) && (*status != ERROR) && (*status != End_of_Work) ){

        if ( argc - 1 > N ) {
            n++;
            argv = (char**)realloc( argv, sizeof(char*) * n * N );
            if ( argv == NULL ){
                fprintf( stderr, "%s", "Not Enough Memory\n" );
                *status = ERROR;
                *pend = end;
                return argv;
            }
        }

        if ( (end == '>') || (end == '<') ){
            argv[argc] = (char*)malloc( 2 * sizeof(char) );
            argv[argc][0] = end;
            argv[argc][1] = '\0';
            argc++;
        }

        argv[argc] = new_arg( status, &end );
        if ( ( (*status == METPALKA) || (*status == ENDINPUT) ) && ( argv[argc][0] == '\0' ) ){
            free( argv[argc] );
            argc--;
        }
        argc++;

    }
    argv[argc] = NULL;
    *pend = end;

    return argv;

}

void test_memory_error( char* s ){
    if (s == NULL){
        fprintf( stderr, "%s", "Not Enough Memory\n" );
        exit(-1);
    }
}

void Syntax_error ( data* files ){
    files->status = ERROR;
    fprintf( stderr, "%s", "Syntax error\n" );
}

void fill_struct( data* files, char*** argv ){

    int i;

    files->input = NULL;
    files->output = NULL;
    files->outputcur = NULL;
    files->error = NULL;

    files->argv = *argv;

    i = 0;
    while( (*argv)[i] != NULL ) {
        if ( (strcmp( (*argv)[i], ">") == 0) || (strcmp( (*argv)[i], "<") == 0) ) break;
        if (strcmp( (*argv)[i], "2") == 0){
            if ((*argv)[i+1] != NULL ){
                if ( strcmp((*argv)[i+1], ">") == 0 ) break;
            }
        }
        i++;
    }

    while( (*argv)[i] != NULL ){

        if ( strcmp( (*argv)[i], "<") == 0 ){

            if ( (*argv)[i+1] == NULL ) {
                Syntax_error( files );
                return;
            }
            if ( ((strcmp( (*argv)[i+1], ">") == 0) || (strcmp( (*argv)[i+1], "<") == 0) || (strcmp( (*argv)[i+1], "2") == 0)) || (files->input != NULL) ) {
                Syntax_error( files );
                return;
            }

            files->input = (char*)malloc( (strlen((*argv)[i+1]) + 1) * sizeof(char) );
            test_memory_error(files->input);
            strcpy( files->input, (*argv)[i+1] );

            free( (*argv)[i] );
            free( (*argv)[i+1] );
            (*argv)[i] = NULL;
            i = i + 2;

            continue;

        }
        if ( strcmp( (*argv)[i], "2") == 0 ){

            if ( (*argv)[i+1] == NULL ) {
                Syntax_error( files );
                return;
            }
            if ( (strcmp( (*argv)[i+1], ">") != 0) || ((*argv)[i+2] == NULL) || (files->error != NULL) ){
                Syntax_error( files );
                return;
            }

            files->error = (char*)malloc( (strlen((*argv)[i+2]) + 1) * sizeof(char) );
            test_memory_error(files->error);
            strcpy( files->error, (*argv)[i+2] );

            free( (*argv)[i] );
            free( (*argv)[i+1] );
            free( (*argv)[i+2] );
            (*argv)[i] = NULL;
            i = i + 3;

            continue;

        }
        if ( strcmp((*argv)[i], ">") == 0 ){

            if ( (files->output != NULL) || (files->outputcur != NULL) || ((*argv)[i+1] == NULL) ){
                Syntax_error( files );
                return;
            }
            if ( strcmp( (*argv)[i+1], ">") == 0 ){// outputcur

                if ( (*argv)[i+2] == NULL ){
                    Syntax_error( files );
                    return;
                }
                if ( (strcmp( (*argv)[i+2], ">") == 0) || (strcmp( (*argv)[i+2], "<") == 0) || (strcmp( (*argv)[i+2], "2") == 0) ){
                    Syntax_error( files );
                    return;
                }
                files->outputcur = (char*)malloc( (strlen((*argv)[i+2]) + 1) * sizeof(char) );
                strcpy( files->outputcur, (*argv)[i+2] );

                free( (*argv)[i] );
                free( (*argv)[i+1] );
                free( (*argv)[i+2] );
                (*argv)[i] = NULL;
                i = i + 3;

                continue;

            }
            else{ // output

                if ( (strcmp( (*argv)[i+1], ">") == 0) || (strcmp( (*argv)[i+1], "<") == 0) || (strcmp( (*argv)[i+1], "2") == 0) ) {
                    Syntax_error( files );
                    return;
                }

                files->output = (char*)malloc( (strlen((*argv)[i+1]) + 1) * sizeof(char) );
                test_memory_error(files->output);
                strcpy( files->output, (*argv)[i+1] );

                free( (*argv)[i] );
                free( (*argv)[i+1] );
                (*argv)[i] = NULL;
                i = i + 2;

                continue;

            }
        }

        Syntax_error( files );
        break;

    }

}

void pwd(void)
{
    int len = 50;
    char* buf = (char*)malloc( len * sizeof(char) );
    test_memory_error(buf);

    while(!getcwd(buf,len)){
        len += 50;
        buf = (char*)realloc( buf, len );
        test_memory_error(buf);
    }
    printf( "%s\n", buf );
    free(buf);
    exit(0);
}

int cd(char **argv)
{
    int res;
    if (!argv[1]) res = chdir(getenv("HOME"));
    else if (strlen(argv[1]) == 0) return 1;
    else res = chdir(argv[1]);

    if (res < 0)
    {
        fprintf(stderr, "Directory %s not found\n", argv[1]);
        return 1;
    }
    return 0;
}

void close_one( int* fd ){
    close(fd[0]);
    close(fd[1]);
}

void Clean( data* files ){
    int i=0;

    if ( files -> input != NULL ) free( files -> input );
    if ( files -> output != NULL ) free( files -> output );
    if ( files -> outputcur != NULL ) free( files -> outputcur );
    if ( files -> error != NULL ) free( files -> error );

    if (files->argv != NULL){
        while( (files->argv)[i] != NULL ) {
            free( (files->argv)[i] );
            i++;
        }
        free( files->argv );
    }

}

void clossing_all( int** ch ){
    close(ch[0][0]);
    close(ch[0][1]);
    close(ch[1][0]);
    close(ch[1][1]);
}

void ending( int** ch, data* files, int flag_status ){
    clossing_all( ch );
    Clean( files );
    if (flag_status == Pipe_ERROR ){
        perror( "pipe_error\n" );
        exit(-1);
    }
    if (flag_status == Syntax_ERROR){
        fprintf( stdout, "%s", "Syntax_error\n" );
        exit(-1);
    }
    if (flag_status == End_of_Work) printf( "Shell is closing, bye\n" );
    exit(0);
}

void open_files( int**ch, data* files, int*fdr, int* fdw, int* fde ){

    if (files->input != NULL) *fdr = open( files->input, O_RDONLY, 0644 );
    if ( *fdr == -1 ){
        fprintf( stderr, "File %s not found\n", files->input );
        ending( ch, files, 0 );
    }
    if (files->error != NULL) *fde = open( files->error, O_WRONLY | O_CREAT, 0644 );
    if ( *fde == -1 ){
        fprintf( stderr, "File %s not found\n", files->error );
        ending( ch, files, 0 );
    }
    if (files->output != NULL) *fdw = open( files->output, O_WRONLY | O_CREAT, 0644 );
    if ( *fde == -1 ){
        fprintf( stderr, "File %s not found\n", files->output );
        ending( ch, files, 0 );
    }
    if (files->outputcur != NULL) {
        *fdw = open( files->outputcur, O_WRONLY | O_CREAT, 0644 );
        lseek( *fdw, 0, SEEK_END );
    }
    if ( *fdw == -1 ){
        fprintf( stderr, "File %s not found\n", files->outputcur );
        ending( ch, files, 0 );
    }

    if ( *fdr > 0 ) {
        dup2( *fdr, 0 );
        close(*fdr);
    }
    if ( *fdw > 0 ) {
        dup2( *fdw, 1 );
        close(*fdw);
    }
    if ( *fde > 0 ) {
        dup2( *fde, 2 );
        close(*fde);
    }

}

void smart_konveer(){

    data files;
    char c, end;
    int fd0[2], fd1[2], fdr=0, fdw=0, fde=0, status = START, i=0, statusson, flagson = FIRST;
    int* ch[2];
    char** argv;
    pid_t pid;
    ch[0] = fd0;
    ch[1] = fd1;
    if ( (pipe(ch[0]) == -1) || (pipe(ch[1]) == -1) ) ending( ch, &files, Pipe_ERROR );

    while(1){

        argv = new_argv(&status, &end);
        files.status = status;
        fill_struct( &files, &argv );
        if (files.status == End_of_Work) ending( ch, &files, 0 );
        if ( (files.status == ERROR) ) {
            status = START;
            statusson = FIRST;
            if (end != '\n') {
                if( scanf( "%c", &c ) != EOF ){
                    while( c != '\n' ) if( scanf( "%c", &c ) == EOF ) ending( ch, &files, 0 );
                }
                else ending( ch, &files, 0 );
            }
            Clean(&files);
            continue;
        }
        if (strcmp( (files.argv)[0], "exit" ) == 0) ending( ch, &files, End_of_Work );

        if ( strcmp((files.argv)[0],"cd") == 0 ) {
            cd(files.argv);
            Clean(&files);
            continue;
        }

        pid = fork();
        if ( pid > 0 ){//father
            close_one(ch[i]);
            wait(&statusson);
            /*if (WEXITSTATUS(statusson)){
                if (end != '\n') {
                    if( scanf( "%c", &c ) != EOF ){
                        while( c != '\n' ) if( scanf( "%c", &c ) == EOF ) ending( ch, &files, 0 );
                    }
                    else ending( ch, &files, 0 );
                    status = START;
                    statusson = FIRST;
                }
            }*/
            //if ( files.status == ENDINPUT ) ending( ch, &files, 0 );//dz6
            flagson = 0;
            if (files.status == ENDINPUT) {
                flagson = FIRST;
                status = START;
            }
            if ( pipe(ch[i]) == -1 ) ending( ch, &files, Pipe_ERROR );
            i = 2-i%2-1;
            Clean(&files);
        }
        if ( pid == 0 ){//son
            if (files.status == ERROR) exit(0);
            if (files.status != ENDINPUT) dup2( ch[2-i%2-1][1], 1 );
            if (flagson != FIRST) dup2( ch[i][0], 0 );
            clossing_all(ch);
            open_files( ch, &files, &fdr, &fdw, &fde );
            if ( strcmp((files.argv)[0],"pwd") == 0 ) pwd();
            else execvp( (files.argv)[0], files.argv );
            fprintf( stderr, "Command %s not found\n", (files.argv)[0] );
            Clean(&files);
            exit(0);
        }
        if ( pid  == -1 ) {
            perror( "fork_error\n" );
            ending( ch, &files, 0 );
        }

    }


}

int main(){
    smart_konveer();
    return 0;
}
