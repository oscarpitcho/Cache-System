#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "commands.h"
#include "addr_mng.h"
#include "error.h"

static const size_t MAX_CHAR_NUMBER = 2;
static size_t handle_line_calls = 0;
static const size_t WORD_SIZE = 4;
static const size_t BYTE_SIZE = 1;

static int printline(const command_t *command, FILE *stream)
{
	if (command->order == READ)
		fprintf(stream, "R ");
	else
		fprintf(stream, "W ");
	if (command->type == INSTRUCTION)
		fprintf(stream, "I ");

	else if (command->data_size == WORD_SIZE) // correcteur here assume type = data
		fprintf(stream, "DW ");
	else
		fprintf(stream, "DB ");
	if (command->data_size == 1 && command->order == WRITE)
		fprintf(stream, "0x%02" PRIX32 " ", command->write_data);
	else if (command->order == WRITE)
		fprintf(stream, "0x%08" PRIX32 " ", command->write_data);
	fprintf(stream, "@0x%016" PRIX64 "\n", virt_addr_t_to_uint64_t(&(command->vaddr)));
	return ferror(stream) == 1 ? ERR_IO : ERR_NONE;
}

static void handle_line_instruction(command_t *line)
{
	//In case of instruction read fields data size and write data are chosen to be 0
	line->data_size = WORD_SIZE;
	line->write_data = 0;
	line->type = INSTRUCTION;
}

static void handle_line_data_read(FILE *input, command_t *line, char size)
{
	M_REQUIRE(size == 'W' || size == 'B', ERR_BAD_PARAMETER, "Not a valid size");
	size == 'W' ? (line->data_size = WORD_SIZE) : (line->data_size = BYTE_SIZE);
	line->write_data = 0;
	line->type = DATA;
}

static void handle_line_data_write(FILE *input, command_t *line, char size)
{

	M_REQUIRE(size == 'W' || size == 'B', ERR_BAD_PARAMETER, "Not a valid size");
	size == 'B' ? fscanf(input, " %" SCNx8, &(line->write_data)) : fscanf(input, " %" SCNx32, &(line->write_data));
	size == 'B' ? (line->data_size = BYTE_SIZE) : (line->data_size = WORD_SIZE);
	line->type = DATA;
}

static command_t handle_line(FILE *input)
{
	command_t line;
	char accessType;
	char word_byte[MAX_CHAR_NUMBER];
	fscanf(input, " %c %2s", &accessType, word_byte);
	line.order = accessType == 'R' ? READ : WRITE;
	uint64_t vaddr = 0;

	if (line.order == READ)
	{
		word_byte[0] == 'I' ? handle_line_instruction(&line) : handle_line_data_read(input, &line, word_byte[1]);
	}
	else
	{
		handle_line_data_write(input, &line, word_byte[1]);
	}
	fscanf(input, " %*c%" SCNx64, &vaddr);
	init_virt_addr64(&line.vaddr, vaddr);
	return line;
}

int program_read(const char *filename, program_t *program)
{
	M_REQUIRE_NON_NULL(filename);
	M_REQUIRE_NON_NULL(program);

	// correcteur swap a W for an R is fine
	// correcteur if W with B but data is bigger, truncated
	// correcteur

	FILE *input;
	program_init(program);
	input = fopen(filename, "r");
	int error = ERR_NONE;
	M_REQUIRE_NON_NULL_CUSTOM_ERR(input, ERR_IO);

	do
	{
		command_t lineCo = handle_line(input);
		error = program_add_command(program, &lineCo); // correcteur error is still not checked (the variable)
	} while (!feof(input) && !ferror(input));
	if (ferror(input))
	{
		fclose(input);
		return ERR_IO;
	}
	fclose(input);
	if (program->nb_lines != 0)
		program->nb_lines--;
	error = program_shrink(program);
	return ferror(input) ? ERR_IO : error;
}
int program_init(program_t *program)
{
	M_REQUIRE_NON_NULL(program);

	if ((program->listing = calloc(START_SIZE, sizeof(command_t))) != NULL)
	{
		program->allocated = START_SIZE * sizeof(command_t);
		program->nb_lines = 0;
		return ERR_NONE;
	}
	return ERR_MEM;
}

int program_print(FILE *output, const program_t *program)
{
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(output);
	for_all_lines(line, program)
	{
		if (printline(line, output) == ERR_IO)
			return ERR_IO;
	}
	return ERR_NONE;
}

int program_shrink(program_t *program)
{
	M_REQUIRE_NON_NULL(program);
	if (program->listing != NULL && program->nb_lines > 0 && (program->listing = realloc(program->listing, sizeof(command_t) * program->nb_lines)) != NULL)
	{
		program->allocated = program->nb_lines * sizeof(command_t);
	}
	else
	{

		int error = program_init(program);
		if (error != ERR_NONE)
			return error;
	}
	return ERR_NONE;
}

int program_add_command(program_t *program, const command_t *command)
{
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(command);

	bool wrongSize = (command->data_size != sizeof(word_t) && command->type == INSTRUCTION) || (command->type == DATA && command->data_size != sizeof(word_t) && command->data_size != 1);
	bool writingInstruction = command->type == INSTRUCTION && command->order == WRITE;
	bool invalidAddr = ((command->vaddr).page_offset % (uint16_t)command->data_size) != 0;
	bool wrongWriteData = (command->order == READ && command->write_data != 0);
	//bool wrongAligned = (command->vaddr).page_offset % 4 != 0;

	// correcteur can be diff than READ / WRITE
	// corecteur can be diff than DATA / INSTRUCTION
	//???

	if (wrongSize || writingInstruction || invalidAddr || wrongWriteData)
	{
		return ERR_BAD_PARAMETER;
	}
	if (program->nb_lines * sizeof(command_t) == program->allocated)
	{
		if ((program->listing = realloc(program->listing, sizeof(command_t) * 2 * program->allocated)) == NULL)
		{
			return ERR_MEM;
		}
		program->allocated *= 2;
	}

	program->listing[(program->nb_lines)++] = *command;

	return ERR_NONE;
}

int program_free(program_t *program)
{

	M_REQUIRE_NON_NULL(program);
	if (program->listing != NULL)
	{
		free(program->listing);
		program->listing = NULL;
	}
	program->nb_lines = 0;
	program->allocated = 0;
	return ERR_NONE;
}
