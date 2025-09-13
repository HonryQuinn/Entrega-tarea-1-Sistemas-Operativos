# Tarea 1 Sistemas Operativos
## Sistema de comunicaciòn


#### 1.-) Iniciaciòn de Terminales:
 ```
gcc  -o servidor servidor.c
gcc -o reporte reporte.c
gcc -o usuario usuario.c
```

------------


#### 2.-) Abrir 4 terminales distintas
Una terminal para el servidor, otra terminal para el reportes, y 2 enfocadas en usuarios, en el caso de querer mas usuarios, abrir mas terminales.

- Servidor: Una terminal para iniciar el servidor.

- Reportes: Una terminal dedicada a reportes.

- Usuarios: Dos terminales iniciales para usuarios.

Si se desea simular más usuarios, simplemente abra una terminal adicional para cada usuario extra.

------------


#### 3.-) Orden de ejecuciòn de los programas
Iniciar el Servidor: En una terminal, ejecuta el siguiente comando.
 ```
./servidor
```
Iniciar el Proceso de Reportes: En una terminal separada, ejecuta el comando para los reportes.
 ```
./reporte
```
Conectar Usuarios: Para cada usuario que desees conectar, abre una nueva terminal y usa el comando.

 ```
./usuario
```

------------


#### 4.-) Funciones de usuarios1
##### A-) Sistema de comunicaciòn:
1. Primero, crea ambos usuarios en terminales separadas para asegurarte de que ambos estén activos.

2. Una vez que los usuarios esten listos,  selecciona la opción 1 para empezar a chatear.

3. Usa el PID del otro usuario para establecer la conexión.

4. Proceder a mandar mensajes.

#####  B.) Creaciòn de nuevo usuario:

Para crear otro usuario, presiona 2. Se abrirá una nueva terminal, y este nuevo usuario tendrá su propio PID y operará de forma independiente.

#####  C.-) Sistema de reportes:

El sistema de reportes funciona registrando el PID del usuario reportado. Para verificar los reportes, puedes:

   1- Observar el conteo en las terminales del servidor y de reportes, donde el número incrementa.

   2- Consultar el archivo reportes.txt, que muestra el conteo de cada usuario.

Nota: Es posible que el conteo en las terminales aparezca intercalado, pero el archivo reportes.txt siempre reflejará la cantidad correcta.

Cuando un usuario llega a los 10 reportes, su proceso se cierra automáticamente.

#####  D.-) Opciòn de salida

Al seleccionar la opción 4, el usuario cierra su proceso y se desconecta del servidor.

------------

## ¿Como funciona los programas?
El sistema de comunicación "Embrace the Sun" es una arquitectura cliente-servidor que utiliza FIFOs (Named Pipes) para la comunicación entre procesos:

#### Servidor Central (servidor.c)

-  Actúa como el núcleo del sistema de comunicación
- Gestiona todas las conexiones de usuarios mediante un FIFO global (mi_fifo)
- Procesa cuatro tipos de operaciones: 
-- Mensajes de chat entre usuarios
-- Reportes de usuarios problemáticos
-- Conexiones de nuevos usuarios
-- Desconexiones del sistema
- Inicia automáticamente un proceso externo para el manejo de reportes

#### Clientes/Usuarios (usuario.c)

- Cada usuario se ejecuta como un proceso independiente
- Se conecta al servidor a través del FIFO global
- Crea su propio FIFO personal para recibir mensajes (fifo_[PID])
- Ofrece un menú interactivo con 4 opciones:
--Chat: Comunicación en tiempo real con otros usuarios
--Crear nuevo usuario: Lanza una nueva terminal con otro cliente
--Reportar PID: Envía reportes sobre usuarios problemáticos
-- Salir: Desconexión limpia del sistema

#### Sistema de Reportes (reporte.c)

- Proceso independiente que maneja los reportes de usuarios
- Utiliza un FIFO dedicado (reportes_fifo) para recibir reportes
- Lleva un contador de reportes por usuario en reportes.txt
- Elimina automáticamente usuarios que excedan el límite de reportes (10)
- Notifica las eliminaciones mediante procesos hijo

------------



###End
