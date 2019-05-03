#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <dlfcn.h>

#define EXEVFILENAME "ExevLanceloiaTempFile.c"
#define FUNCFILENAME "FuncLanceloiaTempFile.c"
#define TESTFILENAME "TestLanceloiaTempFile.c"
#define TEMPLIBRNAME "LibrLanceloiaTempFile.so"
#define EXEVFUNCNAME "exevLanceloiaTempFunction"

static void CLOSE() {
  printf("exit\n");
  remove(EXEVFILENAME);
  remove(FUNCFILENAME);
  remove(TESTFILENAME);
  remove(TEMPLIBRNAME);
}

char Template1[] = "int "EXEVFUNCNAME"() {\n  return ";
char Template2[] = ";\n}";

static void Ctrl_C_Handler(){
  printf("\nKeyboard interrupt is disabled. [Quit] Ctrl + D\n>>> ");
  fflush(stdout);
}

int main(int argc, char *argv[], char *env[]) {
  // register disposal function
  atexit(CLOSE);
  signal(SIGINT, Ctrl_C_Handler);
  
  system("clear");
#if defined(__i386__) 
  printf("Crepl 1.0.0 (i386) (copyright: Lanceloia)\n");
#elif defined(__x86_64__)
  printf("Crepl 1.0.0 (x86_64) (copyright: Lanceloia)\n");
#endif

  // Create tempfile's fp
  FILE *exevfp = fopen(EXEVFILENAME, "w");
  FILE *funcfp = fopen(FUNCFILENAME, "w");
  FILE *testfp = fopen(TESTFILENAME, "w");
  if(exevfp == NULL || funcfp == NULL || testfp == NULL) return 1;
  else {fclose(exevfp); fclose(funcfp); fclose(testfp);}

  char buf[1024];
  char buf2[1024];

  // Catch input
  while(printf(">>> ") && fgets(buf, 1024, stdin)){
    buf[strlen(buf) - 1] = '\0';

    // Write tempfile
    if(buf[0] == 'i' && buf[1] == 'n' && buf[2] == 't') {
      // testfp
      testfp = fopen(TESTFILENAME, "w");
      if(testfp == NULL) return 1;
      fprintf(testfp, "%s\n", buf);
      fclose(testfp);

      // complie and add
      int files[2];
      if(pipe(files) != 0) return 1;

      int pid = fork();
      if(pid == 0) {
        // complie
        dup2(files[1], STDERR_FILENO);
        close(files[0]);
#if defined(__i386__) 
        char *newargv[]={"gcc", "-m32", "-fPIC","-shared", TESTFILENAME, "-o", TEMPLIBRNAME, NULL};
#elif defined(__x86_64__)
        char *newargv[]={"gcc", "-fPIC","-shared", TESTFILENAME, "-o", TEMPLIBRNAME, NULL};
#endif
        execve("/usr/bin/gcc", newargv, env);
        assert(0);
      }
      else {
        // add
        wait(NULL);
        
        // catch compile error
        close(files[1]);
        int cnt = 0, ret = 0;
        while((ret = read(files[0], buf2, 1024)) && ret) cnt += ret;
        if(cnt != 0){ printf("Compile Error\n"); continue; }
     
        // add new function
        funcfp = fopen(FUNCFILENAME, "a");
        if(funcfp == NULL) return 1;
        fprintf(funcfp, "%s\n", buf);
        fclose(funcfp);
        printf("Added: %s\n", buf);
      }
    }
    else {
      // build program
      exevfp = fopen(EXEVFILENAME, "w+");

      // add functions
      funcfp = fopen(FUNCFILENAME, "r");
      if(exevfp == NULL || funcfp == NULL) return 1;
      while(fgets(buf2, 1024, funcfp)) fprintf(exevfp, "%s", buf2);
      fclose(funcfp); 

      // add statement
      fprintf(exevfp, "%s%s%s\n", Template1, buf, Template2);
      fclose(exevfp);

      // complie and execve
      int files[2];
      if(pipe(files) != 0) return 1;

      int pid = fork();
      if(pid == 0) {
        // complie
        dup2(files[1], STDERR_FILENO);
        close(files[0]);
#if defined(__i386__) 
        char *newargv[]={"gcc", "-m32", "-fPIC","-shared", EXEVFILENAME, "-o", TEMPLIBRNAME, NULL};
#elif defined(__x86_64__)
        char *newargv[]={"gcc", "-fPIC","-shared", EXEVFILENAME, "-o", TEMPLIBRNAME, NULL};
#endif
        execve("/usr/bin/gcc", newargv, env);
        assert(0);
      }
      else {
        // execve
        wait(NULL);
        
        // catch complie error
        close(files[1]);
        int cnt = 0, ret = 0;
        while((ret = read(files[0], buf2, 1024)) && ret) cnt += ret;
        if(cnt != 0){ printf("Compile error\n"); continue; }

        // exevce statement
        char *error;
        // Open dynamic link file
        void *dlhandle = dlopen("./" TEMPLIBRNAME, RTLD_LAZY);
        if ((error = dlerror()) != NULL) {fprintf(stderr, "%s\n", error); return 1;}
        // load the exevfunc
        int (*exevfunc)() = dlsym(dlhandle, EXEVFUNCNAME);
        if ((error = dlerror()) != NULL) {fprintf(stderr, "%s\n", error); return 1;}
        // output result
        printf("(%s) == %d\n", buf, exevfunc());
        dlclose(dlhandle);
      }  
    } // end else
  } // end while

  return 0;
}
