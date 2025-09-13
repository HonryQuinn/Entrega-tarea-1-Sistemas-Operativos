#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define FIFO_FILE "mi_fifo"

typedef struct {
    pid_t pid_emisor;
    pid_t pid_destino;
    char usuario[32];
    char mensaje[256];
    pid_t pid;
    int tipo; 
} paquete;

pid_t proceso_lector = 0;

void terminar_proceso_lector() {
    if (proceso_lector > 0) {
        kill(proceso_lector, SIGTERM);
        waitpid(proceso_lector, NULL, 0);
        proceso_lector = 0;
    }
}

int verificar_servidor_activo() {
    int fd = open(FIFO_FILE, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        return 0;
    }
    close(fd);
    return 1;
}
// creo que si se puede usar :s
void crear_nuevo_usuario() {
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Creando nuevo usuario...\n");
        
        if (execvp("gnome-terminal", (char*[]){"gnome-terminal", "--", "./usuario", NULL}) == -1) {
            perror("Error");
            exit(1);
        }
        perror("Error ejecutando cliente");
        exit(1);
        
    } else if (pid > 0) {
        printf("Nuevo usuario creado (PID: %d)\n", pid);

    } else {
        perror("Error creando proceso");
    }
}


int cliente_main() {
    pid_t mi_pid = getpid();
    int fd_global, fd_personal;
    paquete p;
    char fifo_name[64];
    int opcion;

    printf("\nSistema 'Embrace the Sun'\n");
    printf("Cliente PID: %d\n", mi_pid);
    
    if (!verificar_servidor_activo()) {
        printf("Error: Servidor no disponible\n");
        return 1;
    }
   
    printf("Ingrese su nombre: ");
    fgets(p.usuario, sizeof(p.usuario), stdin);
    p.usuario[strcspn(p.usuario, "\n")] = 0;

    snprintf(fifo_name, sizeof(fifo_name), "fifo_%d", mi_pid);
    if (mkfifo(fifo_name, 0666) == -1 && errno != EEXIST) {
        perror("Error creando FIFO personal");
        exit(1);
    }

    fd_global = open(FIFO_FILE, O_RDWR);
    if (fd_global == -1) {
        perror("Error conectando al servidor");
        exit(1);
    }

    fd_personal = open(fifo_name, O_RDWR | O_NONBLOCK);
    if (fd_personal == -1) {
        perror("Error abriendo FIFO personal");
        exit(1);
    }

    printf("Cliente %s conectado\n", p.usuario);

    paquete conexion;
    conexion.pid_emisor = mi_pid;
    conexion.pid_destino = 0;
    strcpy(conexion.usuario, p.usuario);
    strcpy(conexion.mensaje, "Conectado");
    conexion.tipo = 2;
    write(fd_global, &conexion, sizeof(conexion));

    while (1) {
        if (!verificar_servidor_activo()) {
            printf("Servidor desconectado\n");
            break;
        }
        
        printf("\n=== MENÚ ===\n");
        printf("1.- Chat\n");
        printf("2.- Crear nuevo usuario\n");
        printf("3.- Reportar PID\n");
        printf("4.- Salir\n");
        printf("Opción: ");

        if (scanf("%d", &opcion) != 1) {
            printf("Entrada inválida\n");
            while (getchar() != '\n'); 
            continue;
        }
        getchar(); 

        if (opcion == 1) {
            printf("=== CHAT ===\n");
            
            proceso_lector = fork();
            if (proceso_lector == 0) {
                paquete r;
                printf("Escuchando mensajes...\n");
                
                while (1) {
                    int n = read(fd_personal, &r, sizeof(r));
                    if (n > 0 && r.tipo == 0) {
                        printf("\nMensaje de %s (PID:%d): %s\n", 
                               r.usuario, r.pid_emisor, r.mensaje);
                        printf("Destino (0=todos, -1=salir): ");
                        fflush(stdout);
                    }
                    usleep(50000);
                }
                exit(0);
            }

            while (1) {
                printf("Destino (0=todos, -1=salir): ");
                scanf("%d", &p.pid_destino);
                getchar();

                if (p.pid_destino == -1) {
                    terminar_proceso_lector();
                    break;  
                }

                printf("Mensaje: ");
                if (fgets(p.mensaje, sizeof(p.mensaje), stdin) == NULL) continue;
                
                p.mensaje[strcspn(p.mensaje, "\n")] = 0;
                p.pid_emisor = mi_pid;
                p.tipo = 0;

                if (write(fd_global, &p, sizeof(p)) == -1) {
                    printf("Error enviando mensaje\n");
                    break;
                } else {
                    printf("Mensaje enviado\n");
                }
            }

        } else if (opcion == 2) {
            crear_nuevo_usuario();

        } else if (opcion == 3) {
            printf("PID a reportar: ");
            pid_t pid_reportar;
            scanf("%d", &pid_reportar);
            getchar();

            if (pid_reportar == mi_pid) {
                printf("No puedes reportarte a ti mismo\n");
                continue;
            }

            p.pid_emisor = mi_pid;
            p.pid_destino = 0;
            snprintf(p.mensaje, sizeof(p.mensaje), "reportar %d", pid_reportar);
            p.tipo = 1;

            if (write(fd_global, &p, sizeof(p)) == -1) {
                printf("Error enviando reporte\n");
            } else {
                printf("Reporte enviado\n");
            }

        } else if (opcion == 4) {
            printf("Saliendo...\n");
            break;

        } else {
            printf("Opción inválida\n");
        }
    }

    paquete desconexion;
    desconexion.pid_emisor = mi_pid;
    desconexion.pid_destino = 0;
    strcpy(desconexion.usuario, p.usuario);
    strcpy(desconexion.mensaje, "Desconectado");
    desconexion.tipo = 3;
    write(fd_global, &desconexion, sizeof(desconexion));

    terminar_proceso_lector();
    close(fd_global);
    close(fd_personal);
    unlink(fifo_name);
    
    printf("Cliente desconectado\n");
    return 0;
}

int main() {
    return cliente_main();
}