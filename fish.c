#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cmdline.h"

#define BUFLEN 2048

//Number maximum of commands in background
#define maxCmdBackground 50

//Length max of the name of commands
#define maxLengthNameCmd 20

#define YESNO(i) ((i) ? "Y" : "N")

//Structure
struct cmd_bg{
	pid_t pid;
	char* nom_cmd;
	int num_li;
	bool estTermine;
};

struct TD_commandes_background{
	int nb;
	struct cmd_bg* tab;
};

//Prototype
int max (int a,int b);
int recouvrement(const struct line li);
void fct_background(const struct cmd c,pid_t pid,bool memeLigne);
void redirection_input(const char *file_input, const struct cmd c);
void redirection_output(const char *file_input, const struct cmd c);
void check_status(int st);
void annuleCTRLc();
void remetCTRLc();
void cd (const struct cmd c);
void termine_le_fils(int sig, siginfo_t *info);
void delete_cmd(int rang);
int tubeNcommandes(const struct line li);
void testRetour(int res, char *perror, char *message);
void affiche_background();
bool testLineFinie(int num_li);
void supprimeAttente(int num_li);

//Global structure in the program
struct TD_commandes_background enAttente, aAfficher;

int main() {
  struct line li;
  char buf[BUFLEN];

//strcuture initialization
  enAttente.nb=0;
  aAfficher.nb=0;
  enAttente.tab=calloc(maxCmdBackground,sizeof(struct cmd_bg));
  if(enAttente.tab==NULL){
	perror("calloc");
	exit(1);
  }
  aAfficher.tab=calloc(maxCmdBackground,sizeof(struct cmd_bg));
  if(aAfficher.tab==NULL){
	perror("calloc");
	exit(1);
  }

  line_init(&li);

  //function canceling ctrl+c
  annuleCTRLc();

  for (;;) {
	printf("fish> ");
    char *res = fgets(buf, BUFLEN, stdin);
    if (res == NULL) {
    	perror("fgets");
		fprintf(stderr,"\nFgets error\n");
		exit (1);
    }
    int err = line_parse(&li, buf);
    if (err) { 
      //the command line entered by the user isn't valid
      line_reset(&li);
      continue;
    }
	if(li.ncmds ==0){
		affiche_background();
		continue;
	}
		if(strcmp(li.cmds[0].args[0],"exit")==0){
			for (int i = 0; i < enAttente.nb; ++i) {
				if (enAttente.tab[i].nom_cmd != NULL) {	
					free(enAttente.tab[i].nom_cmd);
				}
			}
			for (int i = 0; i < aAfficher.nb; ++i) {
				if (aAfficher.tab[i].nom_cmd != NULL) {
					free(aAfficher.tab[i].nom_cmd);
				}
			}
			free(aAfficher.tab);
			free(enAttente.tab);
			line_reset(&li);
			exit(0);
		}
		
		fprintf(stderr, "Command line:\n");
		fprintf(stderr, "\tNumber of commands: %zu\n", li.ncmds);

		for (size_t i = 0; i < li.ncmds; ++i) {
		  fprintf(stderr, "\t\tCommand #%zu:\n", i);
		  fprintf(stderr, "\t\t\tNumber of args: %zu\n", li.cmds[i].nargs);
		  fprintf(stderr, "\t\t\tArgs:");
		  for (size_t j = 0; j < li.cmds[i].nargs; ++j) {
		    fprintf(stderr, " \"%s\"", li.cmds[i].args[j]);
		  }
		  fprintf(stderr, "\n");
		}

		fprintf(stderr, "\tRedirection of input: %s\n", YESNO(li.redirect_input));
		if (li.redirect_input) {
		  fprintf(stderr, "\t\tFilename: '%s'\n", li.file_input);
		}

		fprintf(stderr, "\tRedirection of output: %s\n", YESNO(li.redirect_output));
		if (li.redirect_output) {
		  fprintf(stderr, "\t\tFilename: '%s'\n", li.file_output);
		}

		fprintf(stderr, "\tBackground: %s\n", YESNO(li.background));
	if (li.ncmds ==1) {
		if(strcmp(li.cmds[0].args[0],"cd")==0){
			cd(li.cmds[0]);
		} else {
			//Function to do the recovery
			if(!li.background){
				check_status(recouvrement(li));
			}
			else{
				recouvrement(li);
			}
			printf("\n");
		}
	}
	else{
			if(!li.background){
				check_status(tubeNcommandes(li));
			}
			else{
				tubeNcommandes(li);
			}
	}
	affiche_background();
	line_reset(&li);
  }
  return 0;
}

/**
* This function perfoms directory moves using the cd command
*
* @param c struct cmd  structure of the command
* @return when the file or filder doesn't exist
*/
void cd (const struct cmd c){
	if(c.nargs == 1) {
		fprintf(stderr,"Missing options.\n");
		return;
	}
	if (!strcmp(c.args[1], "~")) {
		char buf[BUFLEN];
		if(getcwd(buf,BUFLEN)==NULL){
			perror("getcwd1");
			exit(1);
		}
		printf("Old directory :%s\n",buf);
		int res = chdir(getenv("HOME"));
		if(res!=0){
			perror("chdir");
			fprintf(stderr,"%s : no file or folder of this type.\n",c.args[1]);
			return;
		}
		if(getcwd(buf,BUFLEN)==NULL){
			perror("getcwd2");
			exit(1);
		}
		printf("New directory : %s\n",buf);
		return;
	}
	if(c.nargs<3){
		char buf[BUFLEN];
		if(getcwd(buf,BUFLEN)==NULL){
			perror("getcwd1");
			exit(1);
		}
		printf("Old directory :%s\n",buf);
		int res;		
		if(c.nargs==2){
			res=chdir(c.args[1]);
		}
		else{
			res=chdir(getenv("HOME"));
		}
		if(res!=0){
			perror("chdir");
			fprintf(stderr,"%s : no file or folder of this type.\n",c.args[1]);
			return;
		}
		if(getcwd(buf,BUFLEN)==NULL){
			perror("getcwd2");
			exit(1);
		}
		printf("New directory : %s\n",buf);
	}
	else{
		fprintf(stderr,"Too much argument.");
	}
}

/**
* Function that performs the command entered in background with &. This function
* looks at the signal sigchld to detect when the command is finished
*
* @param c struct cmd  structure of the command
* @param pid pid_t  pid
* @param memeLigne bool  checked if it's the same line
*/
void fct_background(const struct cmd c, pid_t pid, bool memeLigne) {
		if(enAttente.nb==50){
			//to avoid segmentation error, we don't allow to the user to do more than 50 commands in background at the same time.
			fprintf(stderr,"There is too much commands in background");
			exit(1);
		}
		int res;
		struct sigaction action;
		sigemptyset(&action.sa_mask);
		action.sa_handler=(void *)termine_le_fils;
		action.sa_flags=SA_SIGINFO|SA_RESTART;
		res = sigaction(SIGCHLD, &action,NULL);
		testRetour(res,"sigaction","Sigaction error, function fct_background");
		enAttente.tab[enAttente.nb].pid=pid;
		enAttente.tab[enAttente.nb].estTermine=false;
		int num_processus;
		if(enAttente.nb==0&&aAfficher.nb!=0){
			num_processus=aAfficher.tab[aAfficher.nb-1].num_li;
		}
		if(enAttente.nb!=0&&aAfficher.nb==0){
			num_processus=enAttente.tab[enAttente.nb-1].num_li;
		}
		if(enAttente.nb!=0&&aAfficher.nb!=0){
			num_processus=max(enAttente.tab[enAttente.nb-1].num_li,aAfficher.tab[aAfficher.nb-1].num_li);
		}
		if(enAttente.nb==0&&aAfficher.nb==0){
			num_processus=0;
		}
		if(!memeLigne){
			num_processus++;
		}
		enAttente.tab[enAttente.nb].num_li=num_processus;
		if(!memeLigne){
			printf("\n[%d] %d\n",enAttente.tab[enAttente.nb].num_li,pid);
		}
		enAttente.tab[enAttente.nb].nom_cmd=calloc(maxLengthNameCmd,sizeof(char));	
		if(enAttente.tab[enAttente.nb].nom_cmd==NULL){
			perror("calloc");
			exit(1);
		}
		strcpy(enAttente.tab[enAttente.nb].nom_cmd,c.args[0]);
		enAttente.nb++;
}


/**
* This function redirects the input.
*
* @param *file_input const char get the file to open read-only
* @param c struct cmd  structure of the command
*/
void redirection_input(const char *file_input, const struct cmd c){
	int res;
	int ret=open(file_input,O_RDONLY);
	testRetour(ret,"open","Open error, function redirection_input");
	res=dup2(ret,0);
	testRetour(res,"dup2","Dup2 error, function redirection_input");
	res=close(ret);
	testRetour(res,"close","Close error, function redirection_input");
}

/**
* This function redirects the ouput.
*
* The file is open for readind and writing if it exists, else it's created.
*
* @param *file_input const char get the file to open
* @param c struct cmd  structure of the command
*/
void redirection_output(const char *file_output, const struct cmd c){
	int res;
	int ret=open(file_output,O_WRONLY|O_CREAT,0777);
	testRetour(ret,"open","Open error, function redirection_output");
	res=dup2(ret,1);
	testRetour(res,"dup2","Dup2 error, function redirection_output");
	res=close(ret);
	testRetour(res,"close","Close error, function redirection_output");
}


/**
* This function reads the status number received as a parameter and determines whether
* the status is completed normally or not and displays an error message.
*
* @param st int status number received
*/
void check_status(int st) {
	if (WIFEXITED (st)){
		fprintf (stderr,"\nThe command ended normally : status = %d\n",WEXITSTATUS (st)) ;
	}
	else{
		if (WIFSIGNALED (st)){
			fprintf (stderr,"\nThe command ended withe the use of a signal : status = %d signal de fin: %d\n",WEXITSTATUS (st),WTERMSIG(st)) ;
		}
	}
}

/**
* This function cancels the use of ctrl+c to stop the current program
* by using the siganls.
*/
void annuleCTRLc() {
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_handler=SIG_IGN;
	action.sa_flags=0;
	
	int res = sigaction(SIGINT, &action, NULL);
	testRetour(res,"sigaction","Sigaction error, function annuleCTRLc");
}

/**
* This function resets the ctl+c signal for a command launched in the shell.
*/
void remetCTRLc() {
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_handler=SIG_DFL;
	action.sa_flags=0;
	int res = sigaction(SIGINT, &action, NULL);
	testRetour(res,"sigaction","Sigaction error, function remetCTRLc");
}

/**
* This function reads a line, the command and its options and performs the
* recovery of the command using function fork().
*
* @param li struct line structure of the line
* @return status int return the status of the recovery
*/
int recouvrement(const struct line li) {
	pid_t pid = fork();
	int status = 1;
	int res;
	if (pid == 0) {
		if (li.redirect_input) {
			redirection_input(li.file_input,li.cmds[0]);
		}
		else{
			if(li.background){
				redirection_input("/dev/null",li.cmds[0]);
			}
		}
		if (li.redirect_output) {
			redirection_output(li.file_output,li.cmds[0]);
		}
		
		remetCTRLc();
		
		res= execvp(li.cmds[0].args[0], li.cmds[0].args);
		testRetour(res,"execvp","Command doesn't exist, function recouvrement.");
		exit(1);
	} else {
		if(li.background){
			fct_background(li.cmds[0],pid,false);
		}
		else{
			res=wait(&status);
			testRetour(res,"wait","Wait error, function recouvrement");
		}
	}
	return status;
}

/**
* This function terminates the current child and removes the background commands that
* are terminated and cancels the use of ctrl+c.
*
* @param sig int signal number
* @param *info siginfo_t
*/
void termine_le_fils(int sig, siginfo_t *info){
	annuleCTRLc();
	for(int i=0;i<enAttente.nb;i++){
		if(enAttente.tab[i].pid==info->si_pid){
			int res=waitpid(info->si_pid,NULL,0);
			enAttente.tab[i].estTermine=true;
			if(testLineFinie(enAttente.tab[i].num_li)){
				supprimeAttente(enAttente.tab[i].num_li);
			}
			testRetour(res,"waitpid","Waitpid error, function termine_le_fils");
		}
	}
}

/**
* This function removes a command that was waiting from an index and decreases
* the number of waiting files.
*
* @param rang int index from which to delete waiting files
*/
void delete_cmd(int rang){
	for(int i=rang;i<enAttente.nb-1;i++){
		enAttente.tab[i]=enAttente.tab[i+1];
	}
	enAttente.nb--;
}


/**
* This function makes a command with n tube.
*
* @param li struct line structure of the line
*/
int tubeNcommandes(const struct line li){
	int tube[li.ncmds-1][2];
	int status;
	int res;
	for(int i=0;i<li.ncmds;i++){
		res=pipe(tube[i]);
		testRetour(res,"pipe","Erreur pipe, fonction tubeNcommandes");
	}
	pid_t pid=fork();
	
	if(pid==0){
		if (li.redirect_input) {
			redirection_input(li.file_input,li.cmds[0]);
		}
		else{
			if(li.background){
				redirection_input("/dev/null",li.cmds[0]);
			}
		}
		remetCTRLc();
		res=dup2(tube[0][1],1);
		testRetour(res,"dup2","Erreur dup2, fonction tubeNcommandes");
		res=close(tube[0][1]);
		testRetour(res,"close","Erreur close, fonction tubesncommandes");
		res=close(tube[0][0]);
		testRetour(res,"close","Erreur close, fonction tubesncommandes");
		res = execvp(li.cmds[0].args[0], li.cmds[0].args);
		testRetour(res,"execvp","La commande n'existe pas, fonction tubesncommandes. (Commande numero 1)");
		exit(1);
	}
	res=close(tube[0][1]);
	testRetour(res,"close","Erreur close, fonction tubesncommandes");
	if(li.background){
		fct_background(li.cmds[0],pid,false);
	}
	for(int i=1;i<li.ncmds-1;i++){
		pid=fork();
		if(pid==0){
			remetCTRLc();
			res=dup2(tube[i-1][0],0);
			testRetour(res,"dup2","Erreur dup2, fonction tubesncommandes");
			res=close(tube[i-1][0]);
			testRetour(res,"close","Erreur close, fonction tubesncommandes");
			res=dup2(tube[i][1],1);
			testRetour(res,"dup2","Erreur dup2, fonction tubesncommandes");
			res=close(tube[i][1]);
			testRetour(res,"close","Erreur close, fonction tubesncommandes");
			res=execvp(li.cmds[i].args[0], li.cmds[i].args);
			testRetour(res,"execvp","La commande n'existe pas, fonction tubesncommandes.");
			exit(1);
		}
		res=close(tube[i-1][0]);
		testRetour(res,"close","Erreur close, fonction tubesncommandes");
		res=close(tube[i][1]);
		testRetour(res,"close","Erreur close, fonction tubesncommandes");
		if(li.background){
			fct_background(li.cmds[i],pid,true);
		}
		
	}
	pid=fork();
	if(pid==0){
		if (li.redirect_output) {
			redirection_output(li.file_output,li.cmds[1]);
		}
		remetCTRLc();
		res=dup2(tube[li.ncmds-2][0],0);
		testRetour(res,"dup2","Erreur dup2, fonction tubesncommandes");
		res=close(tube[li.ncmds-2][0]);
		testRetour(res,"close","Erreur close, fonction tubesncommandes");
		res = execvp(li.cmds[li.ncmds-1].args[0], li.cmds[li.ncmds-1].args);
		testRetour(res,"execvp","La commande n'existe pas, fonction tubesncommandes. (Commande numero 3)");
		exit(1);
	}

	res=close(tube[li.ncmds-2][0]);
	testRetour(res,"close","Erreur close, fonction tubesncommandes");
	if(li.background){
		fct_background(li.cmds[li.ncmds-1],pid,true);
	}
	else{
		for(int i=0;i<li.ncmds;i++){
			res=wait(&status);
			testRetour(res,"wait","Erreur wait, fonction tubesncommandes");
		}
	}
	return status;
}

/**
* This function tests the returns of functions when the res parameter is smaller
* than 0.
*
* @param res int  return value
* @param *message_perror char  message for the perror
* @param *message char  message for the fprintf
*/
void testRetour(int res, char *message_perror, char *message){
	if(res<0){
		perror(message_perror);
		fprintf(stderr,"\n%s\n",message);
		exit (1);
	}
}
bool testLineFinie(int num_li){
	int res=true;
	for(int i=0;i<enAttente.nb;i++){
		if(enAttente.tab[i].num_li==num_li){
			res=res&&enAttente.tab[i].estTermine;
		}
	}
	return res;
}

/**
* This function deletes the waiting commands when they are in background and
* when they are finished.
*
* @param num_li int  index of the line to delete
*/
void supprimeAttente(int num_li){
	//int nb_suppression=0;
	for(int i=0;i<enAttente.nb;i++){
		if(enAttente.tab[i].num_li==num_li){
			aAfficher.tab[aAfficher.nb].num_li=num_li;
			aAfficher.tab[aAfficher.nb].nom_cmd=enAttente.tab[i].nom_cmd;
			aAfficher.tab[aAfficher.nb].pid=enAttente.tab[i].pid;
			aAfficher.nb++;
			delete_cmd(i);
			i--;
		}
	}
}

/**
* This functions displays background commands which are finished.
*/
void affiche_background(){
	int num_li_prec=-1;
	for(int i=0;i<aAfficher.nb;i++){
		if(num_li_prec!=aAfficher.tab[i].num_li){
			printf("\n[%d] Fini   ",aAfficher.tab[i].num_li);
		}
		else{
			printf("|");
		}
		num_li_prec=aAfficher.tab[i].num_li;
		printf("%s",aAfficher.tab[i].nom_cmd);
		free(aAfficher.tab[i].nom_cmd);
	}
	printf("\n");
	aAfficher.nb=0;
}

/**
* This function returns the biggest number entered as a parameter.
*
* @param a int  number
* @param b int  number
* @return in return the biggest number
*/
int max (int a,int b){
	if(a>b)return a;
	return b;
}
