#include <stdio.h>
#include <string.h>

#include "ai_network.h"
#include "network_data.h"
#include "avs_base.h"

/* Global handle to reference the instantiated NN */
static ai_handle network = AI_HANDLE_NULL;
static ai_buffer ai_input[AI_NETWORK_IN_NUM] = { AI_NETWORK_IN_1 };
static ai_buffer ai_output[AI_NETWORK_OUT_NUM] = { AI_NETWORK_OUT_1 };

static ai_float output1[AI_NETWORK_OUT_1_SIZE];
static ai_float input[AI_NETWORK_IN_1_SIZE];

#ifdef printf
#undef printf
#endif
#define printf AVS_TRACE_INFO


/* Global buffer to handle the activations data buffer - R/W data */
AI_ALIGNED(4)
static ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

void aiLogErr(const ai_error err, const char *fct)
{
	if (fct)
		printf("E: AI error (%s) - type=%d code=%d\r\n", fct,
				err.type, err.code);
	else
		printf("E: AI error - type=%d code=%d\r\n", err.type, err.code);
}

int aiInit(void) {
	ai_error err;
	ai_network_report report;
	ai_bool res;

	const ai_network_params params = AI_NETWORK_PARAMS_INIT(AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
                                                            AI_NETWORK_DATA_ACTIVATIONS(activations));

	err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
	if (err.type != AI_ERROR_NONE) {
		aiLogErr(err, __func__);
		return -1;
	}

	res = ai_network_get_info(network, &report);
	if (res) {
		/* display/use the reported data */
		printf("I: Model Name : %s", report.model_name);
		printf("I: Model Signature : %s", report.model_signature);
		printf("I: Model Date : %s", report.model_datetime);
		printf("I: Compile Date : %s", report.compile_datetime);
		printf("I: Runtime revision : %s", report.runtime_revision);
		printf("I: Runtime version : %d-%d", report.runtime_version.major, report.runtime_version.minor);
		printf("I: Tool revision : %d", report.tool_revision);
		printf("I: Tool version : %d-%d", report.tool_version.major, report.tool_version.minor);
		printf("I: Tool api version : %d-%d", report.tool_api_version.major, report.tool_api_version.minor);
		printf("I: API version : %d-%d", report.api_version.major, report.api_version.minor);
		printf("I: Interface API version : %d-%d", report.interface_api_version.major, report.interface_api_version.minor);
		printf("I: Number of MACC : %d", report.n_macc);
	}

	/* initialize network */
	if (!ai_network_init(network, &params)) {
		err = ai_network_get_error(network);
		aiLogErr(err, __func__);
		return -1;
	}
	return 0;
}

int aiRun(const ai_float *in_data, ai_float *out_data, const ai_u16 batch_size)
{
	ai_i32 nbatch;
	ai_error err;

	/* initialize input/output buffer handlers */
	ai_input[0].n_batches = batch_size;
	ai_input[0].data = AI_HANDLE_PTR(in_data);
	ai_output[0].n_batches = batch_size;
	ai_output[0].data = AI_HANDLE_PTR(out_data);
	nbatch = ai_network_run(network, &ai_input[0], &ai_output[0]);
	if (nbatch != batch_size) {
		err = ai_network_get_error(network);
		/* manage the error */
		aiLogErr(err, __func__);
		return -1;
	}
	return 0;
}

int ai_normalizeFeatures(ai_float * nfeat, char * feat, int nb_features)
{
    // should be a standard std compute but we know data is either 0 or 255 so we
    // simplify the calculation
    int i = 0;
    for (i=0 ; i < nb_features; i++)
        nfeat[i] = feat[i] / 255;
}


// DisplaySymbol : to verify image is the one expected by the NN.
// Example : if the symbol is T, you should get following image
//
//         xxxxxxxxxxxxxxx
//               xx
//               xx
//               xx
//               xx
//               xx
//               xx
//
int displaySymbol(char * symbol) {
	int i = 0;
	char str[5]="";
	char str1[512]="";

    // Display input data in the console
	for (i=0; i<28; i++ ){
		str1[0]='\0';
		for ( int j=0; j < 28; j++) {
			if (symbol[i*28+j] == 255)
				sprintf( str, " ");
			else
				sprintf( str, "x");
			strcat(str1, str);
		}
		printf("%s", str1);
	}
}

int ai_Predict(char *features)
{
	int index = 0;
	ai_u32 label;
	ai_u32 found_label;
	ai_float max_value;

	if (aiInit() < 0) {
		printf("Init FAILURE : go out");
		return -1;
	}

    // Normalize data so that each input is in the range [0.0 - 1.0]
    ai_normalizeFeatures(input, features, AI_NETWORK_IN_1_SIZE );

    // Execute the inference
	if (aiRun(input, output1, 1) < 0) {
		printf("Run FAILURE : go out");
		return -1;
	}

    // Identify most probable symbol
	max_value = output1[0];
	found_label = 0;
	for (index = 0; index < AI_NETWORK_OUT_1_SIZE; index ++) {
		if (output1[index] > max_value) {
			max_value = output1[index];
			found_label = index;
		}
	}
	return found_label;
}
