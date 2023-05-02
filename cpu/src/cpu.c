#include "../include/cpu.h"

int main(void) {
	logger = log_create("cpu.log", "cpu_main", 1, LOG_LEVEL_INFO);
	logger_obligatorio = log_create("cpu_obligatorio.log", "cpu_obligatorio", 1, LOG_LEVEL_INFO);
	config = config_create("cpu.config");
	registros = inicializar_registro();

	if(config == NULL){
		log_error(logger, "No se encontró el archivo :(");
		exit(1);
	}
	leer_config();

	// Inicio de servidor
	fd_cpu = iniciar_servidor(logger, IP, PUERTO);

	// Conexion Kernel
	pthread_t conexion_kernel;
	pthread_create(&conexion_kernel, NULL, (void*) server_escuchar, NULL);
	pthread_detach(conexion_kernel);

	// Conecto CPU con memoria
	int fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
	enviar_mensaje("Hola, soy CPU!", fd_memoria);

	terminar_programa(logger, config);
	return 0;
}

void leer_config(){
	IP = config_get_string_value(config, "IP_ESCUCHA");
	PUERTO = config_get_string_value(config, "PUERTO_ESCUCHA");
	IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
}

// --------------- COMUNICACION ---------------

static void procesar_conexion() {
	op_code cop;
	while (socket_cliente != -1) {
        if (recv(socket_cliente, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, "El cliente se desconecto de %s server", server_name);
            return;
        }
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger, socket_cliente);
			break;
		case CONTEXTO_EJECUCION:
			//lo necesario para recibir el contexto
			break;
		// ...
		default:
			log_error(logger, "Algo anduvo mal en el server de %s", server_name);
			return;
		}
	}

	log_warning(logger, "El cliente se desconecto de %s server", server_name);
	return;
}

void server_escuchar() {
	char *server_name = "CPU";
	socket_cliente = esperar_cliente(logger, server_name, fd_cpu);

	if (socket_cliente == -1) {
		log_info(logger, "Hubo un error en la conexion del Kernel");
	}
	procesar_conexion();
}


// --------------- CICLO DE INSTRUCCIONES ---------------
t_registros* inicializar_registro(){
	registros = malloc(sizeof(t_registros));

	//FUNCÓ PERO SUPONGO QUE HAY QUE CAMBIAR EL SIZE OF
	registros->ax = malloc(sizeof(5));
	registros->bx = malloc(sizeof(5));
	registros->cx = malloc(sizeof(5));
	registros->dx = malloc(sizeof(5));
	registros->eax = malloc(sizeof(9));
	registros->ebx = malloc(sizeof(9));
	registros->ecx = malloc(sizeof(9));
	registros->edx = malloc(sizeof(9));
	registros->rax = malloc(sizeof(17));
	registros->rbx = malloc(sizeof(17));
	registros->rcx = malloc(sizeof(17));
	registros->rdx = malloc(sizeof(17));

	registros->ax = NULL;
	registros->bx = NULL;
	registros->cx = NULL;
	registros->dx = NULL;
	registros->eax = NULL;
	registros->ebx = NULL;
	registros->ecx = NULL;
	registros->edx = NULL;
	registros->rax = NULL;
	registros->rbx = NULL;
	registros->rcx = NULL;
	registros->rdx = NULL;

	return registros;
}

void fetch(t_contexto_ejecucion contexto){
	t_instruccion* proxima_instruccion = list_get(contexto.instrucciones, contexto.program_counter);

	decode(proxima_instruccion, contexto);

	contexto.program_counter += 1;
}

void decode(t_instruccion* proxima_instruccion, t_contexto_ejecucion contexto){
	cod_instruccion cod_instruccion = instruccion_to_enum(proxima_instruccion->instruccion);

	switch(cod_instruccion){
	case SET:
		ejecutar_set(proxima_instruccion->parametro1, proxima_instruccion->parametro2);
		break;
	case MOV_IN:
		break;
	case IO:
		break;
	case F_OPEN:
		break;
	case F_CLOSE:
		break;
	case F_SEEK:
		break;
	case F_READ:
		break;
	case F_WRITE:
		break;
	case F_TRUNCATE:
		break;
	case WAIT:
		break;
	case SIGNAL:
		break;
	case CREATE_SEGMENT:
		break;
	case DELETE_SEGMENT:
		break;
	case YIELD:
		ejecutar_yield(contexto);
		break;
	case EXIT:
		ejecutar_exit(contexto);
		break;
	default:
		log_error(logger, "Instruccion no reconocida");
		return;
	}
}

void ejecutar_set(char* registro, char* valor){

	strcat(valor, "\0");

	if(strcmp(registro, "AX") == 0){
		registros->ax = valor;
	}else if(strcmp(registro, "BX") == 0){
		registros->bx = valor;
	}else if(strcmp(registro, "CX") == 0){
		registros->cx = valor;
	}else if(strcmp(registro, "DX") == 0){
		registros->dx = valor;
	}else if(strcmp(registro, "EAX") == 0){
		registros->eax = valor;
	}else if(strcmp(registro, "EBX") == 0){
		registros->ebx = valor;
	}else if(strcmp(registro, "ECX") == 0){
		registros->ecx = valor;
	}else if(strcmp(registro, "EDX") == 0){
		registros->edx = valor;
	}else if(strcmp(registro, "RAX") == 0){
		registros->rax = valor;
	}else if(strcmp(registro, "RBX") == 0){
		registros->rbx = valor;
	}else if(strcmp(registro, "RCX") == 0){
		registros->rcx = valor;
	}else if(strcmp(registro, "RDX") == 0){
		registros->rdx = valor;
	}

	sleep(RETARDO_INSTRUCCION);
	//VER EL TEMA DEL SLEEP PARA EL RETARDO DE LA INSTRUCCION

}

void ejecutar_yield(t_contexto_ejecucion contexto){
	contexto.estado = YIELD;
	// Avisarle al kernel q ponga al proceso asociado al contexto de ejecucion en ready.
	send_cambiar_estado(contexto, socket_cliente);
}

void ejecutar_exit(t_contexto_ejecucion contexto){
	contexto.estado = EXIT;
	send_cambiar_estado(contexto, socket_cliente);
	// Avisarle al kernel q ponga al proceso asociado al contexto de ejecucion en exit.
}

cod_instruccion instruccion_to_enum(char* instruccion){

	if(strcmp(instruccion, "SET") == 0){
		return SET;
	} else if(strcmp(instruccion, "MOV_IN") == 0){
		return MOV_IN;
	} else if(strcmp(instruccion, "IO") == 0){
		return IO;
	} else if(strcmp(instruccion, "F_OPEN") == 0){
		return F_OPEN;
	} else if(strcmp(instruccion, "F_CLOSE") == 0){
		return F_CLOSE;
	} else if(strcmp(instruccion, "F_SEEK") == 0){
		return F_SEEK;
	} else if(strcmp(instruccion, "F_READ") == 0){
		return F_READ;
	} else if(strcmp(instruccion, "F_WRITE") == 0){
		return F_WRITE;
	} else if(strcmp(instruccion, "F_TRUNCATE") == 0){
		return F_TRUNCATE;
	} else if(strcmp(instruccion, "WAIT") == 0){
		return WAIT;
	} else if(strcmp(instruccion, "SIGNAL") == 0){
		return SIGNAL;
	} else if(strcmp(instruccion, "CREATE_SEGMENT") == 0){
		return CREATE_SEGMENT;
	} else if(strcmp(instruccion, "DELETE_SEGMENT") == 0){
		return DELETE_SEGMENT;
	} else if(strcmp(instruccion, "YIELD") == 0){
		return YIELD;
	} else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	} else{
		log_info(logger, "No se reconocio la funcion");
	}
	return -1;
}
