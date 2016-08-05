extern "C" {

#define S_FUNCTION_NAME  SPlantaNivel
#define S_FUNCTION_LEVEL 2

#include "simstruc.h"
#include "O22SIOMM.h"

#define NENTRADAS	10
#define NSALIDAS	9

/*====================*
 * S-function methods *
 *====================*/


/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */
static void mdlInitializeSizes(SimStruct *S)
{
    ssSetNumSFcnParams(S, 1);		// Number of expected parameters

    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
        return;						// Parameter mismatch will be reported by Simulink
    }

    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 1);		// Usado para actualizar las entradas

    if (!ssSetNumInputPorts(S,1)) return;
	ssSetInputPortWidth( S, 0, NENTRADAS );
	ssSetInputPortRequiredContiguous( S, 0, 1 );
	//for( k=0; k<NENTRADAS; k++ )
	//{
	//	ssSetInputPortWidth(S,k,1);
	//	ssSetInputPortDirectFeedThrough(S,k,1);		// Existen llamadas de la entrada en la funcion mdlOutputs
	//	ssSetInputPortRequiredContiguous(S,k,1);	// sacado del ejemplo (?)
	//}
    
    if (!ssSetNumOutputPorts(S,1)) return;
	ssSetOutputPortWidth( S, 0, NSALIDAS );
	//for( k=0; k<NSALIDAS; k++ )
	//{
	//    ssSetOutputPortWidth(S, k, 1);
	//}

    ssSetNumSampleTimes(S, 1);
    ssSetNumRWork(S, 0);			// reserve element in the float vector
    ssSetNumIWork(S, 0);			// reserve element in the int vector
    ssSetNumPWork(S, 1);			// reserve element in the pointers vector
    ssSetNumModes(S, 0);			// to store a C++ object
    ssSetNumNonsampledZCs(S, 0);	// number of states for which a block detects zero crossings

    ssSetOptions(S, 0);				// set the simulation options that this block implements
}


/* Function: mdlInitializeSampleTimes =========================================
 * Abstract:
 *    This function is used to specify the sample time(s) for your
 *    S-function. You must register the same number of sample times as
 *    specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, mxGetScalar(ssGetSFcnParam(S, 0)));	// tiempo de muestreo?
    ssSetOffsetTime(S, 0, 0.0);
}

/* Function: mdlStart =======================================================
 * Abstract:
 *    This function is called once at start of model execution. If you
 *    have states that should be initialized once, this is the place
 *    to do it.
 */
#define MDL_START
#if defined(MDL_START) 
static void mdlStart(SimStruct *S)
{
	O22SnapIoMemMap *Brain;
	long nResult;

	Brain = new O22SnapIoMemMap();
	nResult = Brain->OpenEnet("192.168.6.100", 2001, 10000, 1);
	//mexPrintf("openenet: %d\n",nResult);

	if ( nResult == SIOMM_OK )
	{
		nResult = Brain->IsOpenDone();
		//mexPrintf("	isopendone: %d\n",nResult);
		while ( nResult == SIOMM_ERROR_NOT_CONNECTED_YET )
		{
			nResult = Brain->IsOpenDone();
			//mexPrintf("  isopendone: %d\n",nResult);
		} 
	}

	// Check for error on OpenEnet() and IsOpenDone()
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo realizar la conexion con exito.");
		return;
	}

	// Configuracion para puntos digitales (necesaria!!!!)
	nResult = Brain->SetDigPtConfiguration(20,0x0180,0x0000);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo configurar al calefactor 1.");
		return;
	}
	nResult = Brain->SetDigPtConfiguration(21,0x0180,0x0000);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo configurar al calefactor 3.");
		return;
	}
	nResult = Brain->SetDigPtConfiguration(22,0x0180,0x0000);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo configurar al calefactor 2.");
		return;
	}
	nResult = Brain->SetDigPtConfiguration(23,0x0180,0x0000);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo configurar al agitador.");
		return;
	}
	nResult = Brain->SetDigPtConfiguration(24,0x0180,0x0000);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo configurar la valvula solenoide 1.");
		return;
	}
	nResult = Brain->SetDigPtConfiguration(25,0x0180,0x0000);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo configurar la valvula solenoide 2.");
		return;
	}
	nResult = Brain->SetDigPtConfiguration(26,0x0180,0x0000);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"No se pudo configurar a las luces del laboratorio.");
		return;
	}
	
	ssGetPWork(S)[0] = (void *) Brain;
}
#endif /*  MDL_START */

/* Function: mdlUpdate ========================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block. Generally outputs are placed in the output vector, ssGetY(S).
 */
#define MDL_UPDATE
static void mdlUpdate(SimStruct *S, int_T tid)
{
	/*********************************
	* Entradas :
	*	0:	Variador			(16)
	*	1:	Valvula Solenoide	(13)
	*	2:	Valvula Motorizada	(12)
	*	3:	Calefactor 1		(20)
	*	4:	Calefactor 2		(22)
	*	5:	Calefactor 3		(21)
	*	6:	Agitador			(23)
	*	7:	Valv. Solenoide 1	(24)
	*	8:	Valv. Solenoide 2	(25)
	*	9:	Luces Lab.      	(26)
	*********************************/

	O22SnapIoMemMap *Brain;
	long nResult,tempDig;
	float tempAna;

	Brain = (O22SnapIoMemMap *) ssGetPWork(S)[0];

	const real_T *u = ssGetInputPortRealSignal(S,0);

	// Variador de Frecuencia
	if( u[0]<5 )
		tempAna = 4.0;
	else
		tempAna = 4.0 + (float)u[0]*16.0/100.0;

	nResult=Brain->SetAnaPtValue(16,tempAna);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato del variador de frecuencia.");
		return;
	}

	// Valvula Solenoide
	tempAna = 4.0 + (float)u[1]*16.0/100.0;
	nResult=Brain->SetAnaPtValue(13,tempAna);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato de la valvula solenoide.");
		return;
	}

	// Valvula Motorizada
	tempAna = 4.0 + (float)u[2]*16.0/100.0;
	nResult=Brain->SetAnaPtValue(12,tempAna);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato de la valvula motorizada.");
		return;
	}

	// Calefactor 1
	if( (float)u[3] > 0.5 )
		tempDig = 1;
	else
		tempDig = 0;
	nResult=Brain->SetDigPtState(20,tempDig);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato del calefactor 1.");
		return;
	}

	// Calefactor 2
	if( (float)u[4] > 0.5 )
		tempDig = 1;
	else
		tempDig = 0;
	nResult=Brain->SetDigPtState(22,tempDig);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato del calefactor 2.");
		return;
	}

	// Calefactor 3
	if( (float)u[5] > 0.5 )
		tempDig = 1;
	else
		tempDig = 0;
	nResult=Brain->SetDigPtState(21,tempDig);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato del calefactor 3.");
		return;
	}

	// Agitador
	if( (float)u[6] > 0.5 )
		tempDig = 1;
	else
		tempDig = 0;
	nResult=Brain->SetDigPtState(23,tempDig);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato del agitador.");
		return;
	}

	// Valv. Solenoide 1
	if( (float)u[7] > 0.5 )
		tempDig = 1;
	else
		tempDig = 0;
	nResult=Brain->SetDigPtState(24,tempDig);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato de la valvula solenoide 1.");
		return;
	}

	// Valv. Solenoide 2
	if( (float)u[8] > 0.5 )
		tempDig = 1;
	else
		tempDig = 0;
	nResult=Brain->SetDigPtState(25,tempDig);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato de la valvula solenoide 2.");
		return;
	}
	
	// Luces Lab.
	if( (float)u[9] > 0.5 )
		tempDig = 1;
	else
		tempDig = 0;
	nResult=Brain->SetDigPtState(26,tempDig);
	if ( nResult != SIOMM_OK )
	{
		ssSetErrorStatus(S,"Error al transmitir el dato de las luces del laboratorio.");
		return;
	}
}



/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block. Generally outputs are placed in the output vector, ssGetY(S).
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
	/********************************************
	* Salidas :
	*	0:	Nivel Presion 4 (Conico)		(0)
	*	1:	Nivel Presion 3 (Cuadrado)		(1)
	*	2:	Nivel Ultrasonico (Cuadrado)	(2)
	*	3:	Flujo Descarga					(8)
	*	4:	Presion Bomba 1					(9)
	*	8:	Presion Bomba 2					(10)
	*	5:	Temp Estanque Cuadrado			(6)
	*	6:	Temp Estanque Conico			(4)
	*	7:	Temp Estanque Recirculacion		(5)
	********************************************/

	O22SnapIoMemMap *Brain;
	float tempAna;
	long nResult;
	int k;

	Brain = (O22SnapIoMemMap *) ssGetPWork(S)[0];
	real_T *y = ssGetOutputPortRealSignal(S,0);
	
	// Nivel Presion 4 (Conico)
	hola:
	nResult=Brain->GetAnaPtValue(0,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de presion (conico).");
		return;
	}
    y[0] = (real_T)tempAna;

	// Nivel Presion 3 (Cuadrado)
	nResult=Brain->GetAnaPtValue(1,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de presion (recirculacion).");
		return;
	}
    y[1] = (real_T)tempAna;

	// Nivel Ultrasonico (Cuadrado)
	nResult=Brain->GetAnaPtValue(2,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de presion (ultrasonico).");
		return;
	}
    y[2] = (real_T)tempAna;

	// Flujo Descarga
	nResult=Brain->GetAnaPtValue(8,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de flujo.");
		return;
	}
    y[3] = (real_T)tempAna;

	// Presion Bomba 1
	nResult=Brain->GetAnaPtValue(9,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de presion bomba 1.");
		return;
	}
    y[4] = (real_T)tempAna;

	// Temp Estanque Cuadrado
	nResult=Brain->GetAnaPtValue(6,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de temperatura (cuadrado).");
		return;
	}
    y[5] = (real_T)tempAna;

	// Temp Estanque Conico
	nResult=Brain->GetAnaPtValue(4,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de temperatura (conico).");
		return;
	}
    y[6] = (real_T)tempAna;

	// Temp Estanque Recirculacion
	nResult=Brain->GetAnaPtValue(5,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de temperatura (recirculacion).");
		return;
	}
    y[7] = (real_T)tempAna;

    // Presion Bomba 2
	nResult=Brain->GetAnaPtValue(10,&tempAna);
	if ( nResult != SIOMM_OK )
	{
		//mexPrintf("getanaptvalue: %d\n",nResult);
		ssSetErrorStatus(S,"Error al recibir el dato de velocidad.");
		return;
	}
	y[8] = (real_T)tempAna;

}

/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.  For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
	O22SnapIoMemMap *Brain;

	Brain = (O22SnapIoMemMap *) ssGetPWork(S)[0];
	// Variador de Frecuencia
	Brain->SetAnaPtValue(16,4.0);
	// Valvula Solenoide
	Brain->SetAnaPtValue(13,4.0);
	// Valvula Motorizada
	Brain->SetAnaPtValue(12,4.0);
	// Calefactor 1
	Brain->SetDigPtState(20,0);
	// Calefactor 2
	Brain->SetDigPtState(22,0);
	// Calefactor 3
	Brain->SetDigPtState(21,0);
	// Agitador
	Brain->SetDigPtState(23,0);
	// Valv. Solenoide 1
	Brain->SetDigPtState(24,0);
	// Valv. Solenoide 2
	Brain->SetDigPtState(25,0);
	// Luces del Laboratorio
	Brain->SetDigPtState(26,0);
	Brain->Close();

	delete Brain;
}

/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif


} // end of extern "C" scope

