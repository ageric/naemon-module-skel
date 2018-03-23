#include <stdarg.h>
#include <stdio.h>
#include <naemon/naemon.h>

NEB_API_VERSION(CURRENT_NEB_API_VERSION);
/* for some reason not exported by Naemon itself */
extern int __nagios_object_structure_version;
static char **tcm_path_cache;
int chan_opath_checks_id;

struct tcm_parent_struct {
	struct host *hst;
};

static gboolean treewalker(gpointer key, gpointer hst_, gpointer user_data)
{
	struct host *hst = (struct host *)hst_;
	struct tcm_parent_struct *tcm_parent = (struct tcm_parent_struct *)user_data;
	tcm_parent->hst = hst;

	/* we're only getting first parent. Returning TRUE means we break the walk */
	return TRUE;
}

static struct host *get_first_parent(struct host *h)
{
	/*
	 * Must be initialized, otherwise the root host of a
	 * chain will return itself, causing an infinite loop
	 */
	struct tcm_parent_struct tcm_parent = { NULL };

	g_tree_foreach(h->parent_hosts, treewalker, &tcm_parent);
	return tcm_parent.hst;
}

static char *tcm_path(host *leaf, char sep)
{
	host *h = leaf;
	char *ret;
	unsigned int path_len = 0, pos = 0;
	objectlist *stack = NULL, *list, *next;

	for (h = leaf; h; h = get_first_parent(h)) {
		path_len += strlen(h->name) + 1;
		prepend_object_to_objectlist(&stack, h->name);
	}

	ret = nm_malloc(path_len + 1);
	for (list = stack; list; list = next) {
		char *ppart = (char *)list->object_ptr;
		next = list->next;
		free(list);
		ret[pos++] = sep;
		memcpy(ret + pos, ppart, strlen(ppart));
		pos += strlen(ppart);
	}
	ret[pos++] = 0;
	return ret;
}

/* entry point for our query handler queries */
int tcm_qh_handler(int sd, char *query, unsigned int len)
{
	if (!strcmp(query, "help")) {
		nsock_printf_nul(sd, "This is the cool module. Only cool people know about it. Sorry");
		return 0;
	}

	if (!strcmp(query, "dumpcache")) {
		unsigned int i;
		for (i = 0; i < num_objects.hosts; i++) {
			struct host *hst = host_ary[i];
			nsock_printf(sd, "%s == %s\n", hst->name, tcm_path_cache[i]);
		}
		return 0;
	}
	return 0;
}

static int chan_opath_checks(int cb, void *data)
{
	const int red = 0xff0000, green = 0xff00, blue = 0xff, pale = 0x555555;
	unsigned int color;
	check_result *cr;
	host *h;
	const char *name = "_HOST_";
	char *buf = NULL;

	if (cb == NEBCALLBACK_HOST_CHECK_DATA) {
		nebstruct_host_check_data *ds = (nebstruct_host_check_data *)data;

		if (ds->type != NEBTYPE_HOSTCHECK_PROCESSED)
			return 0;

		cr = ds->check_result_ptr;
		h = ds->object_ptr;
		color = red | green;
	} else if (cb == NEBCALLBACK_SERVICE_CHECK_DATA) {
		nebstruct_service_check_data *ds = (nebstruct_service_check_data *)data;
		service *s;

		if (ds->type != NEBTYPE_SERVICECHECK_PROCESSED)
			return 0;
		s = (service *)ds->object_ptr;
		h = s->host_ptr;
		cr = ds->check_result_ptr;
		color = (red | green | blue) ^ pale;
		name = s->description;
	} else {
		return 0;
	}

	nm_asprintf(&buf, "%lu|%s|M|%s/%s|%06X\n", cr->finish_time.tv_sec,
		check_result_source(cr), tcm_path_cache[h->id], name, color);
	nerd_broadcast(chan_opath_checks_id, buf, strlen(buf));
	free(buf);
	return 0;
}

/*
 * Called before and after Naemon has read object config and written its
 * objects.cache and status.log files.
 * We want to setup object lists and such here, so we only care about the
 * case where config has already been read.
 */
static int post_config_init(int cb, void *_ds)
{
	nebstruct_process_data *ds = (nebstruct_process_data *)_ds;
	unsigned int i;

	/* Only initialize when we're about to start the event loop */
	if (ds->type != NEBTYPE_PROCESS_EVENTLOOPSTART) {
		return 0;
	}

	/* do stuff here */
	qh_register_handler("tcm", "The Cool Module", 0, tcm_qh_handler);

	tcm_path_cache = calloc(sizeof(char *), num_objects.hosts);
	for (i = 0; i < num_objects.hosts; i++) {
		struct host *h = host_ary[i];
		tcm_path_cache[i] = tcm_path(h, '/');
	}

	chan_opath_checks_id = nerd_mkchan("tcm", "feed me to 'gource --log-format custom'", chan_opath_checks,
		nebcallback_flag(NEBCALLBACK_HOST_CHECK_DATA) | nebcallback_flag(NEBCALLBACK_SERVICE_CHECK_DATA));

	/* now unregister. We don't care about this callback at runtime */
	neb_deregister_callback(NEBCALLBACK_PROCESS_DATA, post_config_init);

	return 0;
}

/**
 * Init routine. This has to exist in all NEB modules.
 * Called after main config is read, but before object config.
 *
 * If we return anything but zero from this one, execution stops.
 *
 * "flags" is unused and holds nothing useful
 *
 * char *arg is what follows the module loading entry in the main config:
 * "nebmodule=/path/to/us.so <THIS MULTI-WORD PART>".
 * Mostly used to set a few config parameters or point the module to
 * its config file.
 *
 * The "nebmodule *handle" is what we have to pass when registering and
 * de-registering callbacks, so stash it in a global var.
 */
static void *neb_handle;
int nebmodule_init(__attribute__((unused)) int flags, char *arg, nebmodule *handle)
{
	neb_handle = (void *)handle;

	/*
	 * usually, one would want to do something like
	 * read_config_file(arg);
	 * or some such here
	 */

	if (__nagios_object_structure_version != CURRENT_OBJECT_STRUCTURE_VERSION) {
		/*
		 * This sucks, as it means we can't very well interact with objects
		 * that don't look like the compiler thought they would when we were
		 * compiled.
		 */
		return -1;
	}

	nm_log(NSLOG_INFO_MESSAGE, "Random module fooblarg loaded");

	/*
	 * This piece of tricker can be used to override the global
	 * setting "event_broker_options" from the main config.
	 * Assuming admins know which modules they load, this is a
	 * sensible thing to do
	 */
	event_broker_options = BROKER_EVERYTHING;

	/* Make sure we get called prior to entering the eventloop */
	neb_register_callback(NEBCALLBACK_PROCESS_DATA, neb_handle, 0, post_config_init);

	return 0;
}

/**
 * Called prior to us being unloaded. This function has to exist in all modules
 *
 * "reason" can be either NEBMODULE_NEB_SHUTDOWN or NEBMODULE_NEB_RESTART
 */
int nebmodule_deinit(__attribute__((unused)) int flags, __attribute__((unused)) int reason)
{
	return 0;
}
