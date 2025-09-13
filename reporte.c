#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#define FIFO_REPORTES "reportes_fifo"
#define ARCHIVO_REPORTES "reportes.txt"
#define MAX_REPORTES 10

typedef struct {
    pid_t reportado;
    pid_t reportador;
    char motivo[128];
} reporte_paquete;

int secundario = 1;

void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        printf("\n Sistema de reportes cerrando...\n");
        secundario = 0;
    }
}
int contar_reportes_usuario(int usuario_id) {
    FILE *file = fopen(ARCHIVO_REPORTES, "r");
    if (!file) return 0;
    
    int count = 0;
    char linea[256];
    int usuario_reportado;
    
    while (fgets(linea, sizeof(linea), file)) {
        if (sscanf(linea, "%d", &usuario_reportado) == 1) {
            if (usuario_reportado == usuario_id) {
                count++;
            }
        }
    }
    fclose(file);
    return count;
}
void notificar_eliminacion(int usuario_id) {
    int pipefd[2];
    pid_t pid;
    
    if (pipe(pipefd) == -1) {
        perror("Error creando pipe");
        return;
    }
    
    pid = fork();
    if (pid == -1) {
        perror("Error en fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    
    if (pid == 0) {
        close(pipefd[1]);
        
        int usuario_eliminado;
        if (read(pipefd[0], &usuario_eliminado, sizeof(int)) > 0) {
            char fifo_name[64];
            snprintf(fifo_name, sizeof(fifo_name), "fifo_%d", usuario_eliminado);
            unlink(fifo_name);
            
            FILE *log_file = fopen("usuarios_eliminados.log", "a");
            if (log_file) {
                fclose(log_file);
            }
            
            printf(" Usuario %d eliminado del sistema\n", usuario_eliminado);
        }
        
        close(pipefd[0]);
        exit(0);
        
    } else {
        close(pipefd[0]); 
        
        write(pipefd[1], &usuario_id, sizeof(int));
        close(pipefd[1]);
        
        wait(NULL);
    }
}
void procesar_reporte(reporte_paquete r_paquete) {
    int reportes_actuales = contar_reportes_usuario(r_paquete.reportado);
    
    if (reportes_actuales >= MAX_REPORTES) {
        printf("Usuario %d ya fue eliminado\n", r_paquete.reportado);
        return;
    }
    
    FILE *file = fopen(ARCHIVO_REPORTES, "a");
    if (!file) {
        perror("Error abriendo archivo de reportes");
        return;
    }
    
    fprintf(file, "%d\n", r_paquete.reportado);
    fclose(file);
    
    int nuevos_reportes = reportes_actuales + 1;
    printf("Usuario %d reportado por %d. Reportes: %d/%d\n", 
           r_paquete.reportado, r_paquete.reportador, nuevos_reportes, MAX_REPORTES);
    
    if (nuevos_reportes >= MAX_REPORTES) {
        printf(" Usuario %d eliminado por reportes\n", r_paquete.reportado);
        
        kill(r_paquete.reportado, SIGTERM);
        
        notificar_eliminacion(r_paquete.reportado);
    }
}
void segundo_plano() {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    printf("\n=== Sistema de reportes iniciado ===\n");
    printf("PID reportes: %d\n", getpid());
    
    int fd_reportes = open(FIFO_REPORTES, O_RDWR);
    if (fd_reportes == -1) {
        perror("Error abriendo FIFO de reportes");
        exit(1);
    }
    
    while (secundario) {
        reporte_paquete r_paquete;
        int n = read(fd_reportes, &r_paquete, sizeof(r_paquete));
        
        if (n > 0) {
            printf("Reporte recibido: %d reporta a %d (motivo: %s)\n", 
                   r_paquete.reportador, r_paquete.reportado, r_paquete.motivo);
            procesar_reporte(r_paquete);
        } else {
            usleep(100000); 
        }
    }
    
    close(fd_reportes);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
    segundo_plano();
    return 0;
}
 
    if (strcmp(argv[1], "agregar") == 0) {
        if (argc < 5) {
            printf("Uso: %s agregar <reportado> <reportador> \n", argv[0]);
            return 1;
        }
        
        int reportado = atoi(argv[2]);
        int reportador = atoi(argv[3]);
        
    }else {
        return 1;
    }
    
    return 0;
}
