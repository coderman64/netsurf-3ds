NetSurf Integration Testing
===========================

[TOC]

# Overview

The monkey frontend is used to perform complex tests involving
operating the browser as a user might (opening windows, navigating to
websites and rendering the contents etc.)

A test is written as a set of operations in a yaml file. These files
are parsed and executed by the monkey driver script.

There are very few tests within the NetSurf repository. The large
majority of integration tests are instead held within the
[netsurf-test](http://source.netsurf-browser.org/netsurf-test.git/)
repository.

To allow more effective use of these tests additional infrastructure
has been constructed to allow groupings of tests to be run. This is
used extensively by the CI system to perform integration testing on
every commit.

The infrastructure also provides for special CGI scripts which have
known behaviours such as delays or returning specific content to
extend test capabilities.


# Running a test

An individual test can be run using the monkey_driver.py python script
from within the NetSurf repository

    $ make TARGET=monkey
    $ ./test/monkey_driver.py -m ./nsmonkey -t test/monkey-tests/start-stop.yaml

The command actually executed can be augmented using the wrapper
switch, this allows the test to be run under a debugger or profiler.

For example to wrap execution under valgrind memory checker

    $ ./test/monkey_driver.py -m ./nsmonkey -w 'valgrind -v --track-origins=yes' -t test/monkey-tests/start-stop.yaml


# Running more than one test

Each test is a member of a group and the tests within each group are
run together. Groups are listed within division index files. A group
of tests may occur within more than one division.

To run the integration tests the monkey-see-monkey-do python script is
used. It downloads the test plan for a division from the NetSurf test
infrastructure and executes it.

    $ ./test/monkey-see-monkey-do
    Fetching tests...
    Parsing tests...
    Running tests...
    Start group: initial
      [ Basic checks that the browser can start and stop ]
      => Run test: start-stop-no-js.yaml
      => Run test: basic-navigation.yaml
      => Run test: start-stop.yaml
    Start group: no-networking
      [ Tests that require no networking ]
      => Run test: resource-scheme.yaml
    Start group: ecmascript
      [ ECMAScript tests ]
    PASS

If no division is specified on the command line the "default" division
is used. Other divisions are specified with the d switch for example
to specify the "short-internet" division:

    $ ./test/monkey-see-monkey-do -d short-internet

Additionally the g switch allows the limiting of tests within a single
group to be executed.

    $ ./test/monkey-see-monkey-do -g no-networking
    Fetching tests...
    Parsing tests...
    Running tests...
    Start group: no-networking
      [ Tests that require no networking ]
      => Run test: resource-scheme.yaml
    PASS

# Test files

Each test is a individual [YAML](https://en.wikipedia.org/wiki/YAML)
file and consists of associative arrays (key/value pairs), lists and
comments.

As a minimum a test must contain an associative array with keys for
`title`, `group` and `steps`. The `steps` key must contain a list of
test operations as a value.

Each operation is an associative list and must, as a minimum, contain
an action key with a suitable value.

A minimal test that simply starts the browser without JavaScript and
then quits it without ever opening a window to browse to a page

    title: start and stop browser without JS
    group: initial
    steps:
    - action: launch
      options:
      - enable_javascript=0
    - action: quit


# Actions

## launch

Start a browser instance. A test will generally have a single launch
action paired with a quit action.

Additional command line parameters may be set using the `launch-options`
key the value of which must be a list of command line arguments to be
passed to the browser (without their leading hyphens)

The environment of the browser can be altered with the `environment` key
the value is an associative array of environment variables which will
be added to the browsers environment variables.

User options may be set using the `options` key with a value containing
a list of options to set. These options allow operation with differing
user choices to be tested without a separate Choices file.

The `language` key sets the LANGUAGE environment variable which controls
the browsers user interface language. Note this is distinct from the
language the browser requests from HTTP servers which is controlled
with the `accept_language` user option.

The following launch action would start a browser:
 * Passing `--verbose` on the commandline
 * The `NETSURFRES` environment variable set to `/home/netsurf/resources`
 * The user options `enable_javascript` and `send_referer` set to false.
 * The `LANGUAGE` environment variable set to `en`

```
- action: launch
  launch-options:
  - verbose
  environment:
    NETSURFRES: /home/netsurf/resources
  options:
  - enable_javascript=0
  - send_referer=0
  language: en
```

## window-new

Open a new browser window. The test may open as many browser windows
as necessary and they are usually paired with a `window-close` action

The browser must have been previously launched or this action will
assert the test with a failure.

The `tag` key *must* also be present with a value (unique for all
window-new actions). The value is used to identify subsequent
operations in this window.

As an example this will open a new window which can subsequently be
referred to with the win1 identifier:

    - action: window-new
      tag: win1


## window-close

Closes a previously opened window. The window is identified with the
`window` key, the value of this must be a previously created window
identifier or an assert will occur.

    - action: window-close
      window: win1


## navigate

Cause a window to start navigating to a new URL.

The window to be navigated is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

The URL to navigate to navigate to is controlled either by the `url`
or `repeaturl` key. The `url` value is directly used as the address to
navigate to.

    - action: navigate
      window: win1
      url: about:about

The `repeaturl` value is used as a repeat action identifier allowing
navigation in a loop with different values.

    - action: repeat
      values:
      - https://www.google.com/
      - https://apple.com/
      - https://microsoft.com/
      tag: urls
    - action: navigate
      window: win1
      repeaturl: urls


## reload

Cause a window to (re)navigate to the current URL

The window to be navigated is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

    - action: reload
	  window: win1

## stop

Cause a window to immediately stop any navigation.

The window to be navigated is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

    - action: stop
	  window: win1

## sleep-ms

Wait for time to pass before continuing to next action.

The value of the `time` key is either the duration to wait for in
milliseconds or a `repeat` action identifier.

The optional `conditions` key may contain a list of conditionals used
to terminate the delay early. If a `repeat` action identifier is in
use the loop is terminated if a condition is met.

For example to wait 10 seconds:

    - action: sleep-ms
      time: 10000

if a repeat action identifier is used the delay duration is the
current iteration value and the delay is timed from when the current
iteration started.

The `sleep-ms` action here delays by 50 milliseconds more each
iteration until the window navigation is complete when the `sleep-ms`
action is delaying.

    - action: repeat
      min: 0
      step: 50
      tag: sleepytimer
      steps:
      - action: launch
      - action: window-new
        tag: win1
       - action: navigate
        window: win1
        url: about:about
      - action: sleep-ms
        time: sleepytimer
        conditions:
        - window: win1
          status: complete
      - action: quit


## block

Wait for conditions to be met before continuing. This is similar to
the `sleep-ms` action except that it will wait forever for the
conditions to be met.

The `conditions` key must contain a list of conditionals used to
terminate the block.

    - action: block
      conditions:
      - window: win1
        status: complete

valid `status` values are `complete` or `loading`.

## repeat

Repeat a set of actions.

The identifier of the repeat action is set with the `tag` key and must
be present and unique among `repeat` action identifiers.

The actions to be repeated are placed in the `steps` list which may
include any action and behaves just like the top level list.

An iterator context is created for the repeat loop. The iterator can
either be configured as a numeric value or as a list of values.

The numeric iterator is controlled with the `min` ,`step` and `max`
keys. All these keys are integer values and their presence is
optional.

The `min` value is the initial value of the iterator which defaults
to 0.

The `step` value controls how much the iterator is incremented
on every loop with default value of 1.

The loop terminates if the `max` value is exceeded. If no `max` value
is specified the loop is infinite (i.e. no default) but may still be
terminated by the `sleep-ms` action

    - action: repeat
      min: 0
      step: 50
      max: 100
      tag: loopvar
      steps:
      - action: launch
      - action: quit

A value iterator has a `values` key containing a list. On each
iteration of the loop a new value is available and can be used by the
`navigate` action. 

Note that `min` ,`step` and `max` are ignored if there is a `values` key

    - action: repeat
      values:
      - https://www.google.com/
      - https://www.blogger.com/
      - https://apple.com/
      - https://microsoft.com/
      tag: urls
      steps:
      - action: navigate
        window: win1
        repeaturl: urls
      - action: block
        conditions:
        - window: win1
          status: complete


## timer-start

Start a timer.

The identifier for the timer is set with the `timer` key.

    - action: timer-start
      timer: timer1


## timer-restart

Re-start a timer

The identifier for the timer is set with the `timer` key.

    - action: timer-restart
      timer: timer1


## timer-stop

Stop a timer

The identifier for the timer is set with the `timer` key.

    - action: timer-stop
      timer: timer1


## timer-check

Check a timer meets a condition.

The identifier for the timer is set with the `timer` key.

The conditional is set with the `condition` key which must be present.


## plot-check

Perform a plot of a previously navigated window.

The window to be rendered is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

The `area` key allows control of the area to be redraw. The parameters are on two forms:

 * A sequence of four numbers in the form `x0 y0 x1 y1`
 * The keyword extent which attempt to plot the entire extent of the canvas

An optional list of checks may be specified with the `checks` key. If
any check is not satisfied an assert will occur and the test will
fail. Multiple checks can be specified and all most pass successfully.

The checks available are:

 * The key `text-contains` where the text must occur somewhere in the
   plotted output.
 * The key `text-not-contains` where the text must not occur in the
   plotted output.
 * The key `bitmap-count` which specifies the number of images that
   must be present.


    - action: plot-check
      window: win1
	  area: extent
      checks:
      - text-contains: NetSurf
      - text-contains: Browser
	  - text-not-contains: Chrome
      - bitmap-count: 1


## click

Perform a user mouse click on a specified window.

The window to be clicked is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

The `target` key contains an associative array which is used to select
the location of the mouse operation in the window. The key `text` can
be used to select text to be operated upon which matches the first
occurrence of the text. The key `bitmap` has an integer value to
select the index of the image to click.

The optional `button` key selects which button is pressed it can take
the value `left` or `right`. The default if not specified is `left`

The optional `kind` key selects which button operation to be performed
it can take the value `single`, `double` or `triple`. The default if
not specified is `single`

    - action: click
      window: win1
      target:
        text: "about:Choices"


## wait-loading

Wait for the navigated page to start loading before moving to the next
action.

The window to be waited upon is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

    - action: wait-loading
      window: win1


## add-auth

Add basic authentication details for a navigation.

The keys `url`, `realm`, `username` and `password` must be given. When
a basic authentication challenge occurs that matches the url and
realm parameters the associated username and password are returned to
answer the challenge.

    - action: add-auth
      url: http://test.netsurf-browser.org/cgi-bin/auth.cgi?user=foo&pass=bar
      realm: Fake Realm
      username: foo
      password: bar


## remove-auth

Remove a previously added authentication details.

    - action: remove-auth
      url: http://test.netsurf-browser.org/cgi-bin/auth.cgi?user=foo&pass=bar
      realm: Fake Realm
      username: foo
      password: bar


## clear-log

Clear log for a window.

The window to be cleared is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.


## wait-log

Wait for string to appear in log output.

The window to be waited upon is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

The keys `source` `foldable` `level` and `substring` must be specified

## js-exec

Execute javascript in a window.

The window to be execute within is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

The `cmd` key contains the javascript to execute.


## page-info-state

Check the page information status matches an expected value.

The window to be checked is identified with the `window` key, the
value of this must be a previously created window identifier or an
assert will occur.

The value of the `match` key is compared to the windows page
information status and an assert occurs if there is a mismatch.

## quit

This causes a previously launched browser instance to exit cleanly.
