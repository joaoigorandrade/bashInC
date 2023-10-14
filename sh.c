#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* MARK NAME Seu Nome Aqui */
/* MARK NAME Nome de Outro Integrante Aqui */
/* MARK NAME E Etc */

/****************************************************************
 * Shell xv6 simplificado
 *
 * Este codigo foi adaptado do codigo do UNIX xv6 e do material do
 * curso de sistemas operacionais do MIT (6.828).
 ***************************************************************/

#define MAXARGS 10
char *lsArgs[] = {"/bin/ls", NULL};

/* Todos comandos tem um tipo.  Depois de olhar para o tipo do
 * comando, o código converte um *cmd para o tipo específico de
 * comando. */
struct cmd {
  int type; /* ' ' (exec)
               '|' (pipe)
               '<' or '>' (redirection) */
};

struct execcmd {
  int type;             // ' '
  char *argv[MAXARGS];  // argumentos do comando a ser exec'utado
};

struct redircmd {
  int type;         // < ou >
  struct cmd *cmd;  // o comando a rodar (ex.: um execcmd)
  char *file;       // o arquivo de entrada ou saída
  int mode;         // o modo no qual o arquivo deve ser aberto
  int fd;           // o número de descritor de arquivo que deve ser usado
};

struct pipecmd {
  int type;           // |
  struct cmd *left;   // lado esquerdo do pipe
  struct cmd *right;  // lado direito do pipe
};

int fork1(void);               // Fork mas fechar se ocorrer erro.
struct cmd *parsecmd(char *);  // Processar o linha de comando.

void call_exec(const char *command, char *const args[]) {
  if (fork() == 0) {
    if (execvp(command, args) == -1) {
      perror("execvp");
      exit(1);
    }
  } else {
    int status;
    wait(&status);
  }
}

/* Executar comando cmd.  Nunca retorna. */
void runcmd(struct cmd *cmd) {
  int id;
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0) exit(0);

  switch (cmd->type) {
    default:
      fprintf(stderr, "tipo de comando desconhecido\n");
      exit(-1);

    case ' ':
      ecmd = (struct execcmd *)cmd;
      if (ecmd->argv[0] == 0) exit(0);
      /* MARK START task2
       * TAREFA2: Implemente codigo abaixo para executar
       * comandos simples. */
      /* if (strcmp(ecmd->argv[0], "ls") == 0) {
         call_exec("/bin/ls", lsArgs);
       } else if (strcmp(ecmd->argv[0], "cat") == 0) {
         char *catArgs[] = {"/bin/cat", ecmd->argv[1], NULL};
         call_exec("/bin/cat", catArgs);
       } else if (strcmp(ecmd->argv[0], "sort") == 0) {
         char *sortArgs[] = {"/bin/sort", NULL};
         call_exec("/bin/sort", sortArgs);
       } else if (strcmp(ecmd->argv[0], "uniq") == 0) {
         char *sortArgs[] = {"/bin/sort", NULL};
         call_exec("/bin/sort", sortArgs);
       }*/
      call_exec(ecmd->argv[0], ecmd->argv);
      /* MARK END task2 */
      break;

    case '>':
    case '<':
      rcmd = (struct redircmd *)cmd;
      /* MARK START task3
       * TAREFA3: Implemente codigo abaixo para executar
       * comando com redirecionamento. */
      if (rcmd->type == '>') {
        if ((r = open(rcmd->file, rcmd->mode, 0666)) < 0) {
          fprintf(stderr, "open %s has failed\n", rcmd->file);
          exit(0);
        }
      } else if (rcmd->type == '<') {
        if ((r = open(rcmd->file, rcmd->mode)) < 0) {
          fprintf(stderr, "open %s has failed\n", rcmd->file);
          exit(0);
        }
      }
      if (dup2(r, rcmd->fd) < 0) {
        fprintf(stderr, "dup2 has failed\n");
        exit(0);
      }
      /* MARK END task3 */
      runcmd(rcmd->cmd);
      break;

    case '|':
      pcmd = (struct pipecmd *)cmd;
      /* MARK START task4
       * TAREFA4: Implemente codigo abaixo para executar
       * comando com pipes. */
      if (pipe(p) < 0) {
        fprintf(stderr, "pipe has failed\n");
        exit(0);
      }

      if ((id = fork()) < 0) {
        fprintf(stderr, "fork has failed\n");
        exit(0);

      } else if (id == 0) {
        close(p[1]);
        dup2(p[0], STDIN_FILENO);
        runcmd(pcmd->right);
        close(p[0]);

      } else {
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
        runcmd(pcmd->left);
        close(p[1]);
        wait(&id);
      }
      /* MARK END task4 */
      break;
  }
  exit(0);
}

int getcmd(char *buf, int nbuf) {
  if (isatty(fileno(stdin))) fprintf(stdout, "$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if (buf[0] == 0)  // EOF
    return -1;
  return 0;
}

int main(void) {
  static char buf[100];
  int r;

  // Ler e rodar comandos.
  while (getcmd(buf, sizeof(buf)) >= 0) {
    if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') {
      buf[strlen(buf) - 1] = 0;
      if (chdir(buf + 3) < 0) fprintf(stderr, "reporte erro\n");
      continue;
    }

    if (fork1() == 0) runcmd(parsecmd(buf));
    wait(&r);
  }
  exit(0);
}

int fork1(void) {
  int pid;

  pid = fork();
  if (pid == -1) perror("fork");
  return pid;
}

/****************************************************************
 * Funcoes auxiliares para criar estruturas de comando
 ***************************************************************/

struct cmd *execcmd(void) {
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd *)cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, int type) {
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd *)cmd;
}

struct cmd *pipecmd(struct cmd *left, struct cmd *right) {
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

/****************************************************************
 * Processamento da linha de comando
 ***************************************************************/

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int gettoken(char **ps, char *es, char **q, char **eq) {
  char *s;
  int ret;

  s = *ps;
  while (s < es && strchr(whitespace, *s)) s++;
  if (q) *q = s;
  ret = *s;
  switch (*s) {
    case 0:
      break;
    case '|':
    case '<':
      s++;
      break;
    case '>':
      s++;
      break;
    default:
      ret = 'a';
      while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s)) s++;
      break;
  }
  if (eq) *eq = s;

  while (s < es && strchr(whitespace, *s)) s++;
  *ps = s;
  return ret;
}

int peek(char **ps, char *es, char *toks) {
  char *s;

  s = *ps;
  while (s < es && strchr(whitespace, *s)) s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);

/* Copiar os caracteres no buffer de entrada, comeando de s ate es.
 * Colocar terminador zero no final para obter um string valido. */
char *mkcopy(char *s, char *es) {
  int n = es - s;
  char *c = malloc(n + 1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd *parsecmd(char *s) {
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if (s != es) {
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd *parseline(char **ps, char *es) {
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd *parsepipe(char **ps, char *es) {
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if (peek(ps, es, "|")) {
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es) {
  int tok;
  char *q, *eq;

  while (peek(ps, es, "<>")) {
    tok = gettoken(ps, es, 0, 0);
    if (gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch (tok) {
      case '<':
        cmd = redircmd(cmd, mkcopy(q, eq), '<');
        break;
      case '>':
        cmd = redircmd(cmd, mkcopy(q, eq), '>');
        break;
    }
  }
  return cmd;
}

struct cmd *parseexec(char **ps, char *es) {
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  ret = execcmd();
  cmd = (struct execcmd *)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while (!peek(ps, es, "|")) {
    if ((tok = gettoken(ps, es, &q, &eq)) == 0) break;
    if (tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if (argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}

// vim: expandtab:ts=2:sw=2:sts=2

