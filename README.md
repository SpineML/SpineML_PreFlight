SpineML_PreFlight
=================

Simulator independent initial processing for SpineML models.

This code takes a SpineML model, probably created in SpineCreator, and "preflights" it ready for the simulator.

At present, it replaces any FixedProbability connections with explicit connection lists, to "fix" the model, prior to execution. It modifies the model.xml file accordingly and creates new pp_connectionN.bin files.
