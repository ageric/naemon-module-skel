#include <stdarg.h>
#include <stdio.h>
#include <naemon/naemon.h>

NEB_API_VERSION(CURRENT_NEB_API_VERSION);

/*
 * Called before and after Naemon has read object config and written its
 * objects.cache and status.log files.
 * We want to setup object lists and such here, so we only care about the
 * case where config has already been read.
 */
static int post_config_init(int cb, void *_ds)
{
	nebstruct_process_data *ds = (struct program_status_data *)_ds;

	/* Only initialize when we're about to start the event loop */
	if (ds->type != NEBTYPE_PROCESS_EVENTLOOPSTART) {
		return 0;
	}

	/* do stuff here */

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

	nm_log(NSLOG_INFO_MESSAGE, "Random module fooblarg loaded")

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
	unsigned int i;
	return 0;
}
