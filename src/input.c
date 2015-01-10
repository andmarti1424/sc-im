#include <sys/time.h>
#include <string.h>
#include <ctype.h>   // for isdigit
#include <curses.h>
#include <stdlib.h> // for free

#include "screen.h"
#include "maps.h"
#include "cmds.h"
#include "history.h"
#include "conf.h"   // for config values
#include "utils/string.h"
#include "cmds_visual.h"

static int d;              // char read from stdin
int cmd_multiplier = 0;    // Multiplier
int cmd_pending = 0;       // Command pending
int shall_quit;            // Break loop if ESC key is pressed

/* Función que lee de stdin esperando por un comando valido.
   Detalles: Lee caracteres de stdin y los copia a un buffer
   de entrada al sistema. A medida que se llena, se verifica
   si se forma un comando valido. En caso de encontrarlo,
   lo maneja segun corresponda, delegandolo a una funcion
   especifica por cada modo. Luego de un timeout, si no se
   obtiene un comando valido, se hace flush del buffer.
*/
void handle_input(struct block * buffer) {

    // For measuring timeout
    struct timeval start_tv, m_tv;
    gettimeofday(&start_tv, NULL);
    gettimeofday(&m_tv, NULL);
    long msec = (m_tv.tv_sec - start_tv.tv_sec) * 1000L +
                (m_tv.tv_usec - start_tv.tv_usec) / 1000L;

    cmd_multiplier = 0;
    cmd_pending = 0;

    // Mientras no tenga un comando y no se supere el timeout
    while ( ! has_cmd(buffer, msec) && msec <= CMDTIMEOUT ) {

            // if command pending, refresco solo el ef. multiplicador y cmd pending
            if (cmd_pending) {
                print_mult_pend(input_win);
                wrefresh(input_win);
            }

            // Modifico el estado del cursor de acuerdo
            // al modo actual en el cual se está operando
            handle_cursor();

            // Leo un nuevo caracter desde stdin
            if ( (d = wgetch(input_win)) == OKEY_ESC) {
                break_waitcmd_loop(buffer);
                return;
            }

            // Manejo el efecto multiplicador de un comando
            // para el modo NORMAL.
            if ( d != -1 && isdigit(d)
               && ( buffer->value == '\0' || isdigit((char) buffer->value))
               && ( curmode == NORMAL_MODE || curmode == VISUAL_MODE || curmode == EDIT_MODE )
               && ( cmd_multiplier || d != '0' )
               && ( ! atoi(get_conf_value("numeric")))
               ) {
                    cmd_multiplier *= 10;
                    cmd_multiplier += (int) (d - '0');
                    if (cmd_multiplier > MAX_MULTIPLIER) cmd_multiplier = 0;

                    gettimeofday(&start_tv, NULL);
                    msec = (m_tv.tv_sec - start_tv.tv_sec) * 1000L +
                           (m_tv.tv_usec - start_tv.tv_usec) / 1000L;

                    print_mult_pend(input_win);
                    wrefresh(input_win);
                    continue;
            }

            // Actualizo time stamp start para resetear el timeout en cada ciclo
            // (siempre que el modo sea COMMAND, INSERT o EDIT) o bien con cada
            // entrada del usuario
            fix_timeout(&start_tv);

            // Se ha ingresado un caracter especial: BS TAB ENTER HOME END DEL PGUP PGDOWN
            // o se ha ingresado un caracter alfanumerico
            if (is_idchar(d) || d != -1) {
                // Si estoy en modo normal, visual o de edición, cambio el caracter de esquina superior
                // izquierda a "en espera de finalizacion de comando"
                if ( (curmode == NORMAL_MODE && d >= ' ') ||
                     (curmode == EDIT_MODE   && d >= ' ') || 
                     (curmode == VISUAL_MODE && d >= ' ') ) {
                    cmd_pending = 1;
                }

                addto_buf(buffer, d);

                //Reemplazo mapeos del buffer
                replace_maps(buffer);

            }
            
            gettimeofday(&m_tv, NULL);
            msec = (m_tv.tv_sec - start_tv.tv_sec) * 1000L +
                   (m_tv.tv_usec - start_tv.tv_usec) / 1000L;
    } 

    // timeout. no se ha completado un comando.
    if (msec >= CMDTIMEOUT) {

        // Ya no espero por un comando. Saco flag de pantalla si estaba.
        cmd_pending = 0;

        // Reseteo el efecto multiplicador
        cmd_multiplier = 0;

        //limpio segunda linea
        //clr_header(input_win, 1); //comentado el 22/06/2014

    // Si llego hasta aqui es porque debo ejecutar un comando o un mapeo
    } else {

        cmd_pending = 0; // Ya no espero por un comando. Saco flag de pantalla.
        //if (curmode == NORMAL_MODE) show_header(input_win); linea comentada el 08/06

        //limpio segunda linea      // agregado el 22/06/2014
        clr_header(input_win, 1); // agregado el 22/06/2014

        // Manejo comando, repitiendolo tantas veces como el efecto multiplicador indique.
        // Ej. Si por ej. se tipea 2k, en el buffer se graba kk.
        handle_mult( &cmd_multiplier, buffer, msec );
    }

    print_mult_pend(input_win);    // refresco solo el ef. multiplicador y cmd pending

    // para ambos casos hago flush del buffer
    flush_buf(buffer); 
    return;
} 

// Break waiting command loop
void break_waitcmd_loop(struct block * buffer) {
    if (curmode == COMMAND_MODE) {
        del_item_from_history(commandline_history, 0);
        commandline_history->pos = 0;
    } else if (curmode == VISUAL_MODE) {
        exit_visualmode();
    }

    curmode = NORMAL_MODE;

    // Ya no espero por un comando. Saco flag de pantalla si estaba.
    cmd_pending = 0;

    // Reseteo el efecto multiplicador
    cmd_multiplier = 0;

    // limpio la inputline de modo insert y edit
    inputline[0] = '\0';

    flush_buf(buffer); 
    //clr_header(input_win, 0); // comentado el 22/06/2014
    //show_header(input_win);   // comentado el 22/06/2014
    print_mult_pend(input_win); // agregado  el 22/06/2014 refresco solo el ef. multiplicador y cmd pending
    update();                   // comentado el 22/06/2014
    return; 
}

// Funcion que maneja el timeout de espera de comando dependiendo del modo actual
// NO hay timeout para los modos COMANDO, INSERT y EDIT.
void fix_timeout(struct timeval * start_tv) {
    switch (curmode) {
        case COMMAND_MODE:
        case INSERT_MODE:
            gettimeofday(start_tv, NULL);
            break;
        case VISUAL_MODE:
        case EDIT_MODE:
        case NORMAL_MODE:
            if (d != -1) gettimeofday(start_tv, NULL);
    }
    return;
}


// Funcion que recorre un stuffbuff y determina si en el hay un comando valido
// Ej. buffer = "diw"
int has_cmd (struct block * buf, long timeout) {
    int len = get_bufsize(buf);
    if ( !len ) return 0;
    int k, found = 0; 

    struct block * auxb = (struct block *) create_buf();
   
    for (k = 0; k < len; k++) {
        addto_buf(auxb, get_bufval(buf, k));
        if ( is_single_command(auxb, timeout)) { found = 1; break; }
    } 
    erase_buf(auxb);
    auxb = NULL;
    return found;
}

void do_commandmode(struct block * sb);
void do_normalmode (struct block * buf);
void do_insertmode(struct block * sb);
void do_editmode(struct block * sb);
void do_visualmode(struct block * sb);

// Funcion que delega un comando para ser tratado por una funcion especifica para cada modo
void exec_single_cmd (struct block * sb) {
    switch (curmode) {
        case NORMAL_MODE:
            do_normalmode(sb);
            break;
        case INSERT_MODE:
            do_insertmode(sb);
            break;
        case COMMAND_MODE:
            do_commandmode(sb);
            break;
        case EDIT_MODE:
            do_editmode(sb);
            break;
        case VISUAL_MODE:
            do_visualmode(sb);
            break;
    }
    return;
}

// Manejo el comando final a ser ejecutado, agregando al buffer
// el comando tantas veces como el efecto multiplicador indique.
// Ej.: 4 en el efecto multiplicador y "j" como contenido
// del buffer es reemplazado por "jjjj"
void handle_mult(int * cmd_multiplier, struct block * buf, long timeout) {
    int j, k;
    struct block * b_copy = buf;
    int lenbuf = get_bufsize(b_copy);
    if ( ! *cmd_multiplier) *cmd_multiplier = 1;
    
    for (j = 1; j < *cmd_multiplier; j++) {
        for (k = 0; k < lenbuf; k++) {
            addto_buf(buf, b_copy->value);
            b_copy = b_copy->pnext;
        }
    }

    //if (is_single_command(buf, timeout) == EDITION_CMD)
    //    copybuffer(buf, lastcmd_buffer); // save stdin buffer content in lastcmd buffer
    exec_mult(buf, timeout);
    if (*cmd_multiplier > 1) { *cmd_multiplier = 1; update(); }
    *cmd_multiplier = 0;
    
    return;
} 

// Función que maneja la ejecucion de uno o mas comandos
// que pudieran existir, uno a continuacion de otro, en un buffer
// ej. yryr
void exec_mult (struct block * buf, long timeout) {
    int k, res, len = get_bufsize(buf);
    if ( ! len ) return;

    // Primero intento ejecutar todo el contenido del buffer
    if ((res = is_single_command(buf, timeout))) {
        if (res == EDITION_CMD) copybuffer(buf, lastcmd_buffer); // save stdin buffer content in lastcmd buffer
        //cmd_multiplier--;
        exec_single_cmd(buf); 

    // En caso de no poder, se recorre por bloques
    } else { 
        struct block * auxb = (struct block *) create_buf();
        for (k = 0; k < len; k++) {
            addto_buf(auxb, get_bufval(buf, k));

            if ((res = is_single_command(auxb, timeout))) {
                if (res == EDITION_CMD) copybuffer(buf, lastcmd_buffer); // save stdin buffer content in lastcmd buffer
                //cmd_multiplier--;
                exec_single_cmd(auxb);
                flush_buf(auxb);

                // saco de buf los primeros k valores
                k++;
                while ( k-- ) buf = dequeue(buf);
                // y vuelvo a ejecutar
                if (cmd_multiplier == 0) break;
                exec_mult (buf, timeout);
                break;
            } 
        }
        erase_buf(auxb);
        auxb = NULL;
    }
    return;
}
