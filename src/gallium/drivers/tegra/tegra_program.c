#include <stdio.h>

#include "util/u_memory.h"

#include "tgsi/tgsi_dump.h"

#include "tegra_context.h"
#include "tegra_screen.h"
#include "tegra_program.h"

#define unimplemented() printf("TODO: %s()\n", __func__)


static void *
tegra_create_vs_state(struct pipe_context *pcontext,
		      const struct pipe_shader_state *template)
{
	struct tegra_vs_state *so = CALLOC_STRUCT(tegra_vs_state);
	if (!so)
		return NULL;

	so->base = *template;

	if (tegra_debug & TEGRA_DEBUG_TGSI) {
		fprintf(stderr, "DEBUG: TGSI:\n");
		tgsi_dump(template->tokens, 0);
		fprintf(stderr, "\n");
	}

	return so;
}

static void tegra_bind_vs_state(struct pipe_context *pcontext, void *so)
{
	unimplemented();
}

static void tegra_delete_vs_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

static void *
tegra_create_fs_state(struct pipe_context *pcontext,
		      const struct pipe_shader_state *template)
{
	struct tegra_fs_state *so = CALLOC_STRUCT(tegra_fs_state);
	if (!so)
		return NULL;

	so->base = *template;

	return so;
}

static void tegra_bind_fs_state(struct pipe_context *pcontext, void *so)
{
	unimplemented();
}

static void tegra_delete_fs_state(struct pipe_context *pcontext, void *so)
{
	FREE(so);
}

void tegra_context_program_init(struct pipe_context *pcontext)
{
	pcontext->create_vs_state = tegra_create_vs_state;
	pcontext->bind_vs_state = tegra_bind_vs_state;
	pcontext->delete_vs_state = tegra_delete_vs_state;

	pcontext->create_fs_state = tegra_create_fs_state;
	pcontext->bind_fs_state = tegra_bind_fs_state;
	pcontext->delete_fs_state = tegra_delete_fs_state;
}
