# Contract: Swift Runtime API

`BeebMachine` owns one C runtime handle and is `Sendable`. The C++ owner, not a
Swift `NSLock`, serializes machine state. Public methods include lifecycle
state, `start()`, `pause()`, throwing `reset()`, and the existing media/input/
observation operations migrated to structured results.

Every non-OK C status maps to a typed `BeebError` category retaining its
diagnostic. Returned state, frame, and audio are Swift-owned values. No callback
is invoked under runtime synchronization. Deinitialization performs blocking
shutdown; callers must retain the object for concurrent operations.

Swift task-group tests cover concurrent lifecycle, mutation, observation,
failure recovery, and release after all tasks complete.
