SpineML_PreFlight
=================

Simulator independent initial processing for SpineML models.

This code takes a SpineML model, probably created in SpineCreator, and
"preflights" it ready for the simulator.

The program parses the SpineML model, updating any aspects of the
model where parameters, weights or connectivity are specified in
meta-form. For example, where connections are given in fixed
probability form, this program creates a connection list file and
modifies the <FixedProbabilityConnection> xml element into a
<ConnectionList> element with an associated binary connection list
file.

It also replaces those <Property> elements which are state variable
initial values with explicit binary lists.

The original model.xml file is optionally renamed model.xml.bu and a
new model.xml file is written out containing the new, specific
information.

If the user requests "property changes" via the command line, then
this program also modifies the experiment.xml file, adding model
configuration changes there.

The dependency-free, header-only xml parser _rapidxml_ is used to read,
modify and write out XML files.

This code depends on _popt_ (the package may be named _libpopt-dev_ on your OS) for command line option
parsing and on _doxygen_ to build the code documentation. The build system is _GNU autotools_.

##Quick-start guide to compilation

Here's the install procedure for Debian, Ubuntu and similar:

```bash
sudo apt-get install build-essential autoconf doxygen libpopt-dev
cd SpineML_PreFlight
autoreconf -is
./configure --prefix=/usr/local
make -j4
sudo make install
```

Author: Seb James
Licence: GNU GPL
