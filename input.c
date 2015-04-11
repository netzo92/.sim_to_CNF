/**********************************************************************/
/*           This is the input processor for atpg                     */
/*                                                                    */
/*           Author: Tony Hi Keung Ma                                 */
/*           last update : 10/13/1987                                 */
/**********************************************************************/


#include <stdio.h>
#include "global.h"
#include "miscell.h"
#include "stdlib.h"




#define LSIZE   500       /* line length */
#define NUM_OF_INPUT 1500  /* maximum number of inputs */
#define NUM_OF_OUTPUT 1500 /* maximum number of outputs */

int debug = 0;            /* != 0 if debugging */

char *filename;           /* current input file */
int lineno = 0;           /* current line number */
char *targv[100];        /* pointer to tokens on current command line */
int targc;                /* number of args on current command line */
int file_no = 0;         /* number of current file */
wptr *temp_cktin;        /* temporary input wire list */
wptr *temp_cktout;       /* temporary output wire list */

/* eq() routine checks the equivalence of two character strings
 * return TRUE if they are equal, otherwise return FALSE
*/
eq(string1,string2)
register char *string1,*string2;
{
    while(*string1 && *string2) {
        if(*string1++ != *string2++) return(FALSE);
    }
    if (!(*string2)) return(*string1 == '\0');
    return(*string2 == '\0');
}/* end of eq */


int
hashcode(name)
char *name;
{ 
    int i = 0;
    int j = 0;

    while (*name && j < 8) {
        i = i*10 + (*name++ - '0');
        j++;
    }
    i = i%HASHSIZE;
    return(i<0 ? i + HASHSIZE : i);
}/* end of hashcode */


/* nfind checks the existence of a node
 * return the node pointer if it exists; otherwise return a NULL pointer
 */
nptr
nfind(name)
char *name;
{ 
    nptr ntemp;

    for (ntemp= hash_nlist[hashcode(name)]; ntemp != 0; ntemp = ntemp->hnext) {
        if (eq(name,ntemp->name)) return(ntemp);
    }
    return(NULL);
}/* end of nfind */


/* wfind checks the existence of a wire
 * return the wire pointer if it exists; otherwise return a NULL pointer
 */
wptr
wfind(name)
char *name;
{
    wptr wtemp;

    for (wtemp= hash_wlist[hashcode(name)]; wtemp != 0; wtemp = wtemp->hnext) {
        if (eq(name,wtemp->name)) return(wtemp);
    }
    return(NULL);
}/* end of cfind */


/* get wire structure and return the wire pointer */
wptr
getwire(wirename)
char *wirename;
{
    wptr w;
    char *name;
    int hash_no;
    //char *malloc();

    if (w = wfind(wirename)) { return(w); }

    /* allocate new wire from free storage */
    if (!(w = ALLOC(1,struct WIRE))) { error("No more room!"); }
    ncktwire++;

    /* initialize wire entries */

    hash_no = hashcode(wirename);
    w->hnext = hash_wlist[hash_no];
    hash_wlist[hash_no] = w;
    w->pnext = NULL;
    w->name = name =(char *)malloc((strlen(wirename) + 1)*sizeof(char));
    while (*name++ = *wirename++);
    return(w);
}/* end of getwire */


/* get node structure and return the node pointer */
nptr getnode(nodename)
char *nodename;
{
    nptr n;
    char *name;
    int hash_no;
    //char *malloc();
  
    if (n = nfind(nodename)) error("node defined twice");
  
    /* allocate new node from free storage */
    if (!(n = ALLOC(1,struct NODE))) error("No more room!");

    /* initialize node entries */

    hash_no = hashcode(nodename);
    n->hnext = hash_nlist[hash_no];
    hash_nlist[hash_no] = n;
    n->pnext = NULL;
    n->name = name = malloc((strlen(nodename) + 1)*sizeof(char));
    while (*name++ = *nodename++);
    return(n);
}/* end of getnode */

      
/* new gate */
newgate()
{
    nptr n;
    wptr w;
    int i;
    int iwire_index = 0;

    if ((targc < 5) || !(eq(targv[targc - 2],";")))
                error("Bad Gate Record");
    n = getnode(targv[0]);
    ncktnode++;
    n->type = FindType(targv[1]);
    n->nin = targc - 4;
    if (!(n->iwire = ALLOC(n->nin,wptr))) error("No More Room!");
    i = 2;
    while (i < targc) {
        if (eq(targv[i],";")) break;
        w = getwire(targv[i]); 
        n->iwire[iwire_index] = w;
        w->nout++;
        iwire_index++;
        i++;
    }
    if (i == 2 || ((i + 2) != targc)) error("Bad Gate Record");
    i++;
    w = getwire(targv[i]); 
    n->nout = 1;
    if (!(n->owire = ALLOC(1,wptr))) error("No More Room!");
    *n->owire = w;
    w->nin++;
    return;
}/* end of newgate */


set_output()
{
    wptr w;
    int i,j;
        void exit();

    for (i = 1; i < targc; i++) {
        w = getwire(targv[i]);
        for (j = 0; j < ncktout; j++) {
            if (w == temp_cktout[j]) {
                fprintf(stderr,"net %s is declared again as output around line %d\n",w->name,lineno);
                exit(-1);
            }
        }
        temp_cktout[ncktout] = w;
        ncktout++;
    }
    return;
}/* end of set_output */
      

set_input(pori)
int pori;
{
    wptr w;
    int i,j;
        void exit();

    for (i = 1; i < targc; i++) {
         w = getwire(targv[i]);
         for (j = 0; j < ncktin; j++) {
             if (w == temp_cktin[j]) {
                 fprintf(stderr,"net %s is declared again as input around line %d\n",w->name,lineno);
                exit(-1);
            }
        }
        temp_cktin[ncktin] = w;
        if (pori == 1) {
            temp_cktin[ncktin]->flag |= PSTATE;
        }
        ncktin++;
    }
    return;
}/* end of set_input */
      

/* parse input line into tokens, filling up targv and setting targc */
parse_line(line)
char *line;
{
    char **carg = targv;
    char ch;

    targc = 0;
    while (ch = *line++) {
        if (ch <= ' ') continue;
        targc++;
        *carg++ = line-1;
        while ((ch = *line) && ch > ' ') line++;
        if (ch) *line++ = '\0';
    }
    *carg = 0;
}/* end of parse_line */


void
input(infile)
char *infile;
{ 
    int i;
    char line[LSIZE];
    FILE *in;


    for (i = 0; i < HASHSIZE; i++) {
        hash_wlist[i] = NULL;
        hash_nlist[i] = NULL;
    }
    if (!(temp_cktin = ALLOC(NUM_OF_INPUT,wptr)))
                error("No More Room!");
    if (!(temp_cktout = ALLOC(NUM_OF_OUTPUT,wptr)))
                error("No More Room!");
    ncktin = 0;
    ncktout = 0;
    filename = infile;
    if ((in = fopen(infile,"r")) == NULL) {
        fprintf(stderr,"Cannot open input file %s\n",infile);
        exit(-1);
    }
    while (1) {
        if (fgets(line,LSIZE,in) != line) break;
        lineno++;
        parse_line(line);
        if (targv[0] == NULL) continue;
        if (eq(targv[0],"name")) {
            if (targc != 2) error("Wrong Input Format!");
                        continue;
                }
        switch (targv[0][0]) {
            case '#':    break;

            case 'D': debug = 1 - debug; break;

            case 'g': newgate(); break;

            case 'i': set_input(0); break;

            case 'p': set_input(1); break;

            case 'o': set_output(); break;

            case 'n': set_output(); break;

            default:
                fprintf(stderr,"Unrecognized command around line %d in file %s\n",lineno,filename);
                 break;
        }
    }
    fclose(in);
    create_structure();
    fprintf(stdout,"\n");
    fprintf(stdout,"#Circuit Summary:\n");
    fprintf(stdout,"#---------------\n");
    fprintf(stdout,"#number of inputs = %d\n",ncktin);
    fprintf(stdout,"#number of outputs = %d\n",ncktout);
    fprintf(stdout,"#number of gates = %d\n",ncktnode);
    fprintf(stdout,"#number of wires = %d\n",ncktwire);
    if (debug) display_circuit();
}/* end of input */


create_structure()
{
    wptr w;
    nptr n;
    int i, j, k;

    if (!(cktin = ALLOC(ncktin,wptr))) error("No More Room!");
    if (!(cktout = ALLOC(ncktout,wptr))) error("No More Room!");
    for (i = 0; i < ncktin; i++) {
        cktin[i] = temp_cktin[i];
        cktin[i]->flag |= INPUT;
    }
    cfree(temp_cktin);
    for (i = 0; i < ncktout; i++) {
        cktout[i] = temp_cktout[i];
        cktout[i]->flag |= OUTPUT;
    }
    cfree(temp_cktout);

    for (i = 0; i < HASHSIZE; i++) {
        for (w = hash_wlist[i]; w; w = w->hnext) {
                        if (w->nin > 0)
                    if (!(w->inode = ALLOC(w->nin,nptr)))
                                        error("No More Room!");
                        if (w->nout > 0)
                    if (!(w->onode = ALLOC(w->nout,nptr)))
                                        error("No More Room!");
        }
    }
    for (i = 0; i < HASHSIZE; i++) {
        for (n = hash_nlist[i]; n; n = n->hnext) {
            for (j = 0; j < n->nin; j++) {
                for (k = 0; n->iwire[j]->onode[k]; k++);
                n->iwire[j]->onode[k] = n;
            }
            for (j = 0; j < n->nout; j++) {
                for (k = 0; n->owire[j]->inode[k]; k++);
                n->owire[j]->inode[k] = n;
            }
        }
    }
    return;
}/* end of create_structure */


FindType(gatetype)
char *gatetype;
{
    if (!(strcmp(gatetype,"and"))) return(AND);
    if (!(strcmp(gatetype,"AND"))) return(AND);
    if (!(strcmp(gatetype,"nand"))) return(NAND);
    if (!(strcmp(gatetype,"NAND"))) return(NAND);
    if (!(strcmp(gatetype,"or"))) return(OR);
    if (!(strcmp(gatetype,"OR"))) return(OR);
    if (!(strcmp(gatetype,"nor"))) return(NOR);
    if (!(strcmp(gatetype,"NOR"))) return(NOR);
    if (!(strcmp(gatetype,"not"))) {
        if (targc != 5) error("Bad Gate Record");
        return(NOT);
    }
    if (!(strcmp(gatetype,"NOT"))) {
        if (targc != 5) error("Bad Gate Record");
        return(NOT);
    }
    if (!(strcmp(gatetype,"buf"))) {
        if (targc != 5) error("Bad Gate Record");
        return(BUF);
    }
    if (!(strcmp(gatetype,"xor"))) {
        if (targc != 6) error("Bad Gate Record");
        return(XOR);
    }
    if (!(strcmp(gatetype,"eqv"))) {
        if (targc != 6) error("Bad Gate Record");
        return(EQV);
    }
    error("unreconizable gate type");
}/* end of FindType */


/*
* print error message and die
*/
error(message)
char *message;
{
    fprintf(stderr,"%s around line %d in file %s\n",message,lineno,filename);
    exit(0);
}/* end of error */


/*
* get the elapsed cputime
* (call it the first time with message "START")
*/ 
struct tbuffer {
    long utime;
    long stime;
    long cutime;
    long cstime;
} Buffer;

double StartTime, LastTime;

timer(file,mesg1,mesg2)
FILE *file;
char *mesg1,*mesg2;
{
    long junk;
    if(strcmp(mesg1,"START") == 0) {
        junk = times(&Buffer);
        StartTime = (double)(Buffer.utime);
        LastTime = StartTime;
        return;
    }
    junk = times(&Buffer);
    fprintf(file,"#atpg: cputime %s %s: %.1fs %.1fs\n", mesg1,mesg2,
         (Buffer.utime-LastTime)/60.0, (Buffer.utime-StartTime)/60.0);
    LastTime = Buffer.utime;
    return;
}/* end of timer */

display_circuit()
{
    register nptr n;
    register wptr w;
    register int i,j;

    fprintf(stdout,"\n");
    for (i = 0; i < HASHSIZE; i++) {
          for (n = hash_nlist[i]; n; n = n->hnext) {
             fprintf(stdout,"%s ",n->name);
             switch(n->type) {
                case AND :
                   fprintf(stdout,"and ");
                   break;
                case NAND :
                   fprintf(stdout,"nand ");
                   break;
                case OR :
                   fprintf(stdout,"or ");
                   break;
                case NOR :
                   fprintf(stdout,"nor ");
                   break;
                case BUF :
                   fprintf(stdout,"buf ");
                   break;
                case NOT :
                   fprintf(stdout,"not ");
                   break;
                case XOR :
                   fprintf(stdout,"xor ");
                   break;
                case EQV :
                   fprintf(stdout,"eqv ");
                   break;
            }
            for (j = 0; j < n->nin; j++) {
                fprintf(stdout,"%s ",n->iwire[j]->name);
            }
            fprintf(stdout,"; ");
            for (j = 0; j < n->nout; j++) {
                fprintf(stdout,"%s\n",n->owire[j]->name);
            }
        }
    }
    fprintf(stdout,"i ");
    for (i = 0 ; i < ncktin; i++) {
          fprintf(stdout,"%s ",cktin[i]->name);
      }
    fprintf(stdout,"\n");
    fprintf(stdout,"o ");
    for (i = 0 ; i < ncktout; i++) {
          fprintf(stdout,"%s ",cktout[i]->name);
      }
    fprintf(stdout,"\n");

    fprintf(stdout,"\n");
    for (i = 0; i < HASHSIZE; i++) {
          for (w = hash_wlist[i]; w; w = w->hnext) {
            fprintf(stdout,"%s ",w->name);
            for (j = 0; j < w->nin; j++) {
                fprintf(stdout,"%s ",w->inode[j]->name);
             }
            fprintf(stdout,";");
            for (j = 0; j < w->nout; j++) {
                fprintf(stdout,"%s ",w->onode[j]->name);
             }
            fprintf(stdout,"\n");
          }
    }
    return;
} /* end of display_circuit */
//METEHAN OZTEN'S CODE BELOW
cptr make_clause(int size)
{
    cptr my_clause;   
    my_clause = (cptr)malloc(sizeof(cptr));
    my_clause->numbers = (int *)malloc(sizeof(int)*size);
    my_clause->num_numbers = size;
    my_clause->next = NULL; 
   return my_clause;
}

cptr generate_clause(nptr my_node)
{
        int num_inputs = my_node->nin;
        cptr  my_clause;
        if(my_node->type == 3)//AND
        {
                my_clause =  make_clause(num_inputs + 1);
                int i = 0;
                for(i = 0; i < num_inputs; i++)
                {
                        my_clause->numbers[i] = my_node->iwire[i]->gate_num * -1;
                }        
                my_clause->numbers[num_inputs] = my_node->owire[0]->gate_num;
                cptr prev_ptr = my_clause;
                cptr next_ptr;
                for(i = 0; i < num_inputs; i++)
                {
                        next_ptr =  make_clause(2);
                        next_ptr->numbers[0] = my_node->iwire[i]->gate_num;
                        next_ptr->numbers[1] = my_node->owire[0]->gate_num * -1;
                        prev_ptr->next = next_ptr;
                        prev_ptr = prev_ptr->next;
                }        
        }
        else if(my_node->type == 6) //OR
        {
                my_clause = make_clause(num_inputs+1);
                int i = 0;
                for(i = 0; i < num_inputs; i++)
                {
                        my_clause->numbers[i] = my_node->iwire[i]->gate_num;
                }
                my_clause->numbers[num_inputs] = (my_node->owire[0]->gate_num) * (-1);
                cptr prev_ptr = my_clause;
                cptr next_ptr;
                for(i = 0; i < num_inputs; i++){
                        next_ptr = make_clause(2);
                        next_ptr->numbers[0] = (my_node->iwire[i]->gate_num)*(-1);
                        next_ptr->numbers[1] = my_node->owire[0]->gate_num;
                        prev_ptr->next = next_ptr;
                        prev_ptr = prev_ptr->next;
                }
        }   
        else if(my_node->type == 1) //NOT
        {
                my_clause = make_clause(2);
                my_clause->numbers[0] = my_node->iwire[0]->gate_num;
                my_clause->numbers[1] = my_node->owire[0]->gate_num;
                my_clause->next = make_clause(2);
                my_clause->next->numbers[0] = (my_node->iwire[0]->gate_num) *(-1);
                my_clause->next->numbers[1] = (my_node->owire[0]->gate_num) *(-1);
        }
        else if(my_node->type == 2) //NAND
        {
                my_clause = make_clause(num_inputs + 1);
                int i = 0;
                for(i = 0; i < num_inputs; i++)
                        my_clause->numbers[i] = (my_node->iwire[i]->gate_num)*(-1);
                my_clause->numbers[num_inputs] = (my_node->owire[0]->gate_num)*(-1);
                cptr prev_ptr = my_clause;
                cptr next_ptr;
                for(i = 0; i < num_inputs; i++){
                        next_ptr = make_clause(2);
                        next_ptr->numbers[0] = (my_node->iwire[i]->gate_num);
                        next_ptr->numbers[1] = (my_node->owire[0]->gate_num);
                        prev_ptr->next = next_ptr;
                        prev_ptr = prev_ptr->next;
                }
        }
        else if(my_node->type == 5) //NOR
        {
                my_clause = make_clause(num_inputs + 1);
                int i = 0;
                for(i = 0; i <num_inputs; i++)
                        my_clause->numbers[i] = my_node->iwire[i]->gate_num;
                my_clause->numbers[num_inputs] = my_node->owire[0]->gate_num;
                cptr prev_ptr = my_clause;
                cptr next_ptr;
                for(i = 0; i < num_inputs; i++){
                        next_ptr = make_clause(2);
                        next_ptr->numbers[0] = (my_node->iwire[i]->gate_num)*(-1);
                        next_ptr->numbers[1] = (my_node->owire[0]->gate_num)*(-1);
                        prev_ptr->next = next_ptr;
                        prev_ptr = prev_ptr->next;
                }
        }
        return my_clause;
}



void initialize_wires()
{
    int i = 0;
    for(i = 0; i < ncktwire; i++)
        sort_wlist[i]->gate_found = 0;
}
void label_wires_and_collect_nodes(wptr current_wire,int *current_number, nptr *my_nodes, int *node_write)
{

    if(current_wire->gate_found == 1)
        return;
	int my_write_pos;    
	current_wire->gate_num = *current_number;
    current_wire->gate_found = 1;
	(*current_number)++;

	if(current_wire->inode[0]->nin == 0)
	{
		return;
	}
	else
	{
		int i = 0;
        my_write_pos = *node_write;
        (*node_write)++;
		int num_inputs = current_wire->inode[0]->nin;

		for(i = 0; i < num_inputs; i++)
		{
			label_wires_and_collect_nodes(current_wire->inode[0]->iwire[i], current_number, my_nodes, node_write);
		}

		my_nodes[my_write_pos] = current_wire->inode[0];

    }
	return;
}


void write_to_file(cptr base_clause, char *output_file_name, int number_of_clauses, int number_of_literals, int polarity)
{
    FILE *ofile;
    if(polarity >= 0)
        output_file_name[0] = 'p';
    else
        output_file_name[0] = 'n';
    ofile = fopen(output_file_name, "w");
    fprintf(ofile, "c %s \n",output_file_name);
    fprintf(ofile, "c PRIMARY INPUT EQUIVALENCE: ");
    int i = 0;
    for(i = 0; i < ncktin; i++)
    {
            if(cktin[i]->gate_found == 1)
                fprintf(ofile, "%d=%d ",i,cktin[i]->gate_num);
    }
    fprintf(ofile, " \n");
    fprintf(ofile, "p cnf %d %d \n", number_of_literals, number_of_clauses);
    cptr current_clause = base_clause;
  
    for(i = 0; i < number_of_clauses; i++){
        int clause_size = current_clause->num_numbers;
        int y;
        for(y = 0; y < clause_size; y++)
        {
            if(polarity < 0 && i == 0 && y == 0)
                fprintf(ofile, "%d ",(-1)*current_clause->numbers[y]);
            else
                fprintf(ofile, "%d ",current_clause->numbers[y]); 
        }
        fprintf(ofile, "0 \n");
        current_clause = current_clause->next;    
    }
    fclose(ofile);
}

void do_cnf(int current_output, char *output_file_name)
{
    int i;
	nptr *my_nodes = (nptr *) malloc(sizeof(nptr)*ncktnode);
    for(i = 0; i <ncktnode; i++)
		my_nodes[i] = NULL;

	int current_number, node_write;
	int total_clauses;
    current_number = 1;
	node_write = 0;
	wptr current_output_wire = cktout[current_output];
    initialize_wires();
    label_wires_and_collect_nodes(current_output_wire, &current_number, my_nodes, &node_write);
    cptr base_clause = make_clause(1);
    base_clause->numbers[0] = current_output_wire->gate_num;
    cptr prev_clause = base_clause;
    cptr next_clause;
    total_clauses = 1;
    int current_clauses;
    for(i = 0; i < ncktnode ; i++)
    {
        if(my_nodes[i] != NULL){
            next_clause = generate_clause(my_nodes[i]);
    
            prev_clause->next = next_clause;
            while(prev_clause->next != NULL){
                prev_clause = prev_clause->next; 
                total_clauses++;
            }  
        }
    }
    write_to_file(base_clause, output_file_name, total_clauses, current_number-1, 1);
    write_to_file(base_clause, output_file_name, total_clauses, current_number-1, -1);
    return;
}
//METEHAN OZTEN's CODE STARTS BELOW
void setup_cnf(char *inpFile){
    int i_file_len = strlen(inpFile);
    int num_primary_outputs = ncktout;
    if(ncktout > 5)
        num_primary_outputs = 5;
    int x = 0;
    for(x = 0; x < num_primary_outputs; x++){
        char *output_file_name = malloc(sizeof(char)*(i_file_len+4));
        strcpy(output_file_name+3, inpFile);
        output_file_name[0] = 'o';
        char my_num = (char)(((int)'0')+x);
        output_file_name[1] = my_num;
        output_file_name[2] = '_';
        int output_file_len = strlen(output_file_name);   
        if(strcmp(".sim",output_file_name+output_file_len-4) == 0)
        {
            output_file_name[output_file_len-3] = 'c';
            output_file_name[output_file_len-2] = 'n';
            output_file_name[output_file_len-1] = 'f';
        }
        int output_num = 0;
        do_cnf(x,output_file_name);
        free(output_file_name);
    }
}

//END METEHAN OZTEN'S CODE
