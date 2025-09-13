#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define FIFO_GLOBAL "mi_fifo"
#define FIFO_REPORTES "reportes_fifo"

typedef struct {
    pid_t pid_emisor;
    pid_t pid_destino;
    char usuario[32];
    char mensaje[256];
    pid_t pid;
    int tipo; 
} paquete;

typedef struct {
    pid_t reportado;
    pid_t reportador;
    char motivo[128];
} reporte_paquete;

int servidor_activo = 1;
pid_t proceso_reportes_pid = 0;

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nCerrando servidor...\n");
        servidor_activo = 0;
        if (proceso_reportes_pid > 0) {
            kill(proceso_reportes_pid, SIGTERM);
        }
    }
}

void iniciar_proceso_reportes_externo() {
    if (mkfifo(FIFO_REPORTES, 0666) == -1 && errno != EEXIST) {
        perror("Error creando FIFO de reportes");
        return;
    }
    
    proceso_reportes_pid = fork();
    if (proceso_reportes_pid == 0) {
        printf("Iniciando proceso externo de reportes...\n");
        execl("./reporte", "reporte", NULL);
        perror("Error ejecutando proceso de reportes");
        exit(1);
        
    } else if (proceso_reportes_pid > 0) {
        printf("Proceso de reportes externo creado (PID: %d)\n", proceso_reportes_pid);
    } else {
        perror("Error creando proceso de reportes");
    }
}

void procesar_reporte(paquete p) {
    pid_t reportado_pid;
    if (sscanf(p.mensaje, "reportar %d", &reportado_pid) == 1) {
        printf("Procesando reporte: %d reporta a %d\n", p.pid_emisor, reportado_pid);
         
        int fd_reportes = open(FIFO_REPORTES, O_WRONLY | O_NONBLOCK);
        if (fd_reportes != -1) {
            reporte_paquete r_paquete;
            r_paquete.reportado = reportado_pid;
            r_paquete.reportador = p.pid_emisor;
            strcpy(r_paquete.motivo, "Reporte desde chat");
            
            if (write(fd_reportes, &r_paquete, sizeof(r_paquete)) > 0) {
                printf("Reporte enviado al proceso externo\n");
            } else {
                printf("Error \n");
            }
            close(fd_reportes);
        } else {
            printf("Error \n");
        }
    }
}

void broadcast_mensaje(paquete p) {
    printf("[PID %d] %s: %s\n", p.pid_emisor, p.usuario, p.mensaje);
    
    char fifo_destino[64];
    
    if (p.pid_destino == 0) {
        return;
    } else {
        snprintf(fifo_destino, sizeof(fifo_destino), "fifo_%d", p.pid_destino);
        
        int fd_dest = open(fifo_destino, O_RDWR | O_NONBLOCK);
        if (fd_dest != -1) {
            write(fd_dest, &p, sizeof(p));
            close(fd_dest);
        }
    }
}

void procesar_conexion(paquete p) {
    printf("Conectado: %s (PID %d)\n", p.usuario, p.pid_emisor);
    
    char fifo_cliente[64];
    snprintf(fifo_cliente, sizeof(fifo_cliente), "fifo_%d", p.pid_emisor);
    
    if (mkfifo(fifo_cliente, 0666) == -1 && errno != EEXIST) {
        return;
    }
}

void procesar_desconexion(paquete p) {
    printf("Desconectado: PID %d\n", p.pid_emisor);
    
    char fifo_cliente[64];
    snprintf(fifo_cliente, sizeof(fifo_cliente), "fifo_%d", p.pid_emisor);
    unlink(fifo_cliente);
}

int main() {
    signal(SIGINT, signal_handler);
    
    printf("=== Servidor Principal Iniciado ===\n");
    printf("PID Servidor: %d\n", getpid());
    
    iniciar_proceso_reportes_externo();
    
    if (mkfifo(FIFO_GLOBAL, 0666) == -1 && errno != EEXIST) {
        perror("Error creando FIFO global");
        exit(1);
    }
    
    int fd_global = open(FIFO_GLOBAL, O_RDWR);
    if (fd_global == -1) {
        perror("Error abriendo FIFO global");
        exit(1);
    }
    
    printf("Esperando mensajes...\n");
    
    while (servidor_activo) {
        paquete p;
        int n = read(fd_global, &p, sizeof(p));
        
        if (n > 0) {
            switch (p.tipo) {
                case 0: 
                    broadcast_mensaje(p);
                    break;
                case 1: 
                    procesar_reporte(p);
                    break;
                case 2: 
                    procesar_conexion(p);
                    break;
                case 3: 
                    procesar_desconexion(p);
                    break;
            }
        } else {
            usleep(100000);
        }
    }
    
    close(fd_global);
    unlink(FIFO_GLOBAL);
    

    if (proceso_reportes_pid > 0) {
        waitpid(proceso_reportes_pid, NULL, 0);
    }
    
    printf("Servidor cerrado\n");
    return 0;
}
