# Copyright 2017-2019 Daniel Silverstone <dsilvers@digital-scurf.org>
#
# This file is part of NetSurf, http://www.netsurf-browser.org/
#
# NetSurf is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# NetSurf is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Monkey Farmer

The monkey farmer is a wrapper around `nsmonkey` which can be used to simplify
access to the monkey behaviours and ultimately to write useful tests in an
expressive but not overcomplicated DSLish way.  Tests are, ultimately, still
Python code.

"""

# pylint: disable=locally-disabled, missing-docstring

import asyncore
import os
import socket
import subprocess
import time
import errno
import sys

class StderrEcho(asyncore.dispatcher):
    def __init__(self, sockend):
        asyncore.dispatcher.__init__(self, sock=sockend)
        self.incoming = b""

    def handle_connect(self):
        pass

    def handle_close(self):
        # the pipe to the monkey process has closed
        self.close()

    def handle_read(self):
        try:
            got = self.recv(8192)
            if not got:
                return
        except socket.error as error:
            if error.errno == errno.EAGAIN or error.errno == errno.EWOULDBLOCK:
                return
            else:
                raise

        self.incoming += got
        if b"\n" in self.incoming:
            lines = self.incoming.split(b"\n")
            self.incoming = lines.pop()
            for line in lines:
                try:
                    line = line.decode('utf-8')
                except UnicodeDecodeError:
                    print("WARNING: Unicode decode error")
                    line = line.decode('utf-8', 'replace')

                sys.stderr.write("{}\n".format(line))


class MonkeyFarmer(asyncore.dispatcher):

    # pylint: disable=locally-disabled, too-many-instance-attributes

    def __init__(self, monkey_cmd, monkey_env, online, quiet=False, *, wrapper=None):
        (mine, monkeys) = socket.socketpair()

        asyncore.dispatcher.__init__(self, sock=mine)

        (mine2, monkeyserr) = socket.socketpair()

        self._errwrapper = StderrEcho(mine2)

        if wrapper is not None:
            new_cmd = list(wrapper)
            new_cmd.extend(monkey_cmd)
            monkey_cmd = new_cmd

        self.monkey = subprocess.Popen(
            monkey_cmd,
            env=monkey_env,
            stdin=monkeys,
            stdout=monkeys,
            stderr=monkeyserr,
            close_fds=[mine, mine2])

        monkeys.close()
        monkeyserr.close()

        self.buffer = b""
        self.incoming = b""
        self.lines = []
        self.scheduled = []
        self.deadmonkey = False
        self.online = online
        self.quiet = quiet
        self.discussion = []
        self.maybe_slower = wrapper is not None

    def handle_connect(self):
        pass

    def handle_close(self):
        # the pipe to the monkey process has closed
        self.close()

    def handle_read(self):
        try:
            got = self.recv(8192)
            if not got:
                self.deadmonkey = True
                #  ensure the child process is finished and report the exit
                if self.monkey.poll() is None:
                    self.monkey.terminate()
                    self.monkey.wait()
                print("Handling an exit {}".format(self.monkey.returncode))
                print("The following are present in the queue: {}".format(self.lines))
                self.lines.append("GENERIC EXIT {}".format(
                    self.monkey.returncode).encode('utf-8'))
                print("The queue is now: {}".format(self.lines))
                return
        except socket.error as error:
            if error.errno == errno.EAGAIN or error.errno == errno.EWOULDBLOCK:
                return
            else:
                raise

        self.incoming += got
        if b"\n" in self.incoming:
            lines = self.incoming.split(b"\n")
            self.incoming = lines.pop()
            self.lines.extend(lines)

    def writable(self):
        return len(self.buffer) > 0

    def handle_write(self):
        sent = self.send(self.buffer)
        self.buffer = self.buffer[sent:]

    def tell_monkey(self, *args):
        cmd = (" ".join(args))
        if not self.quiet:
            print(">>> {}".format(cmd))
        self.discussion.append((">", cmd))
        cmd = cmd + "\n"
        self.buffer += cmd.encode('utf-8')

    def monkey_says(self, line):
        try:
            line = line.decode('utf-8')
        except UnicodeDecodeError:
            print("WARNING: Unicode decode error")
            line = line.decode('utf-8', 'replace')
        if not self.quiet:
            print("<<< {}".format(line))
        self.discussion.append(("<", line))
        self.online(line)

    def schedule_event(self, event, secs=None, when=None):
        assert secs is not None or when is not None
        if when is None:
            when = time.time() + secs
        self.scheduled.append((when, event))
        self.scheduled.sort()

    def unschedule_event(self, event):
        self.scheduled = [x for x in self.scheduled if x[1] != event]

    def loop(self, once=False):
        if len(self.lines) > 0:
            self.monkey_says(self.lines.pop(0))
            if once:
                return
        while not self.deadmonkey:
            now = time.time()
            while len(self.scheduled) > 0 and now >= self.scheduled[0][0]:
                func = self.scheduled[0][1]
                self.scheduled.pop(0)
                func(self)
                now = time.time()
            if len(self.scheduled) > 0:
                next_event = self.scheduled[0][0]
                asyncore.loop(timeout=next_event - now, count=1)
            else:
                asyncore.loop(count=1)
            while len(self.lines) > 0:
                self.monkey_says(self.lines.pop(0))
                if once or self.deadmonkey:
                    return


class Browser:

    # pylint: disable=locally-disabled, too-many-instance-attributes, dangerous-default-value, invalid-name

    def __init__(self, monkey_cmd=["./nsmonkey"], monkey_env=None, quiet=False, *, wrapper=None):
        self.farmer = MonkeyFarmer(
            monkey_cmd=monkey_cmd,
            monkey_env=monkey_env,
            online=self.on_monkey_line,
            quiet=quiet,
            wrapper=wrapper)
        self.windows = {}
        self.logins = {}
        self.current_draw_target = None
        self.started = False
        self.stopped = False
        self.launchurl = None
        now = time.time()
        timeout = now + 1

        if wrapper is not None:
            timeout = now + 10

        while not self.started:
            self.farmer.loop(once=True)
            if time.time() > timeout:
                break

    def pass_options(self, *opts):
        if len(opts) > 0:
            self.farmer.tell_monkey("OPTIONS " + (" ".join(['--' + opt for opt in opts])))

    def on_monkey_line(self, line):
        parts = line.split(" ")
        handler = getattr(self, "handle_" + parts[0], None)
        if handler is not None:
            handler(*parts[1:])

    def quit(self):
        self.farmer.tell_monkey("QUIT")

    def quit_and_wait(self):
        self.quit()
        self.farmer.loop()
        return self.stopped

    def handle_GENERIC(self, what, *args):
        if what == 'STARTED':
            self.started = True
        elif what == 'FINISHED':
            self.stopped = True
        elif what == 'LAUNCH':
            self.launchurl = args[1]
        elif what == 'EXIT':
            if not self.stopped:
                print("Unexpected exit of monkey process with code {}".format(args[0]))
            assert self.stopped
        else:
            pass

    def handle_WINDOW(self, action, _win, winid, *args):
        if action == "NEW":
            new_win = BrowserWindow(self, winid, *args)
            self.windows[winid] = new_win
        else:
            win = self.windows.get(winid, None)
            if win is None:
                print("    Unknown window id {}".format(winid))
            else:
                win.handle(action, *args)

    def handle_LOGIN(self, action, _lwin, winid, *args):
        if action == "OPEN":
            new_win = LoginWindow(self, winid, *args)
            self.logins[winid] = new_win
        else:
            win = self.logins.get(winid, None)
            if win is None:
                print("    Unknown login window id {}".format(winid))
            else:
                win.handle(action, *args)
                if win.alive and win.ready:
                    self.handle_ready_login(win)

    def handle_PLOT(self, *args):
        if self.current_draw_target is not None:
            self.current_draw_target.handle_plot(*args)

    def new_window(self, url=None):
        if url is None:
            self.farmer.tell_monkey("WINDOW NEW")
        else:
            self.farmer.tell_monkey("WINDOW NEW %s" % url)
        wins_known = set(self.windows.keys())
        while len(set(self.windows.keys()).difference(wins_known)) == 0:
            self.farmer.loop(once=True)
        poss_wins = set(self.windows.keys()).difference(wins_known)
        return self.windows[poss_wins.pop()]

    def handle_ready_login(self, lwin):

        # pylint: disable=locally-disabled, no-self-use

        # Override this method to do useful stuff
        lwin.destroy()


class LoginWindow:

    # pylint: disable=locally-disabled, too-many-instance-attributes, invalid-name

    def __init__(self, browser, winid, _url, *url):
        self.alive = True
        self.ready = False
        self.browser = browser
        self.winid = winid
        self.url = " ".join(url)
        self.username = None
        self.password = None
        self.realm = None

    def handle(self, action, _str="STR", *rest):
        content = " ".join(rest)
        if action == "USER":
            self.username = content
        elif action == "PASS":
            self.password = content
        elif action == "REALM":
            self.realm = content
        elif action == "DESTROY":
            self.alive = False
        else:
            raise AssertionError("Unknown action {} for login window".format(action))
        if not (self.username is None or self.password is None or self.realm is None):
            self.ready = True

    def send_username(self, username=None):
        assert self.alive
        if username is None:
            username = self.username
        self.browser.farmer.tell_monkey("LOGIN USERNAME {} {}".format(self.winid, username))

    def send_password(self, password=None):
        assert self.alive
        if password is None:
            password = self.password
        self.browser.farmer.tell_monkey("LOGIN PASSWORD {} {}".format(self.winid, password))

    def _wait_dead(self):
        while self.alive:
            self.browser.farmer.loop(once=True)

    def go(self):
        assert self.alive
        self.browser.farmer.tell_monkey("LOGIN GO {}".format(self.winid))
        self._wait_dead()

    def destroy(self):
        assert self.alive
        self.browser.farmer.tell_monkey("LOGIN DESTROY {}".format(self.winid))
        self._wait_dead()


class BrowserWindow:

    # pylint: disable=locally-disabled, too-many-instance-attributes, too-many-public-methods, invalid-name

    def __init__(
            self,
            browser,
            winid,
            _for,
            coreid,
            _existing,
            otherid,
            _newtab,
            newtab,
            _clone,
            clone):
        # pylint: disable=locally-disabled, too-many-arguments
        self.alive = True
        self.browser = browser
        self.winid = winid
        self.coreid = coreid
        self.existing = browser.windows.get(otherid, None)
        self.newtab = newtab == "TRUE"
        self.clone = clone == "TRUE"
        self.width = 0
        self.height = 0
        self.title = ""
        self.throbbing = False
        self.scrollx = 0
        self.scrolly = 0
        self.content_width = 0
        self.content_height = 0
        self.status = ""
        self.pointer = ""
        self.scale = 1.0
        self.url = ""
        self.plotted = []
        self.plotting = False
        self.log_entries = []
        self.page_info_state = "UNKNOWN"

    def kill(self):
        self.browser.farmer.tell_monkey("WINDOW DESTROY %s" % self.winid)

    def wait_until_dead(self, timeout=1):
        now = time.time()
        while self.alive:
            self.browser.farmer.loop(once=True)
            if (time.time() - now) > timeout:
                print("*** Timed out waiting for window to be destroyed")
                print("*** URL was: {}".format(self.url))
                print("*** Title was: {}".format(self.title))
                print("*** Status was: {}".format(self.status))
                break

    def go(self, url, referer=None):
        if referer is None:
            self.browser.farmer.tell_monkey("WINDOW GO %s %s" % (
                self.winid, url))
        else:
            self.browser.farmer.tell_monkey("WINDOW GO %s %s %s" % (
                self.winid, url, referer))
        self.wait_start_loading()

    def stop(self):
        self.browser.farmer.tell_monkey("WINDOW STOP %s" % (self.winid))

    def reload(self, all=False):
        all = " ALL" if all else ""
        self.browser.farmer.tell_monkey("WINDOW RELOAD %s%s" % (self.winid, all))
        self.wait_start_loading()

    def click(self, x, y, button="LEFT", kind="SINGLE"):
        self.browser.farmer.tell_monkey("WINDOW CLICK WIN %s X %s Y %s BUTTON %s KIND %s" % (self.winid, x, y, button, kind))

    def js_exec(self, src):
        self.browser.farmer.tell_monkey("WINDOW EXEC WIN %s %s" % (self.winid, src))

    def handle(self, action, *args):
        handler = getattr(self, "handle_window_" + action, None)
        if handler is not None:
            handler(*args)

    def handle_window_SIZE(self, _width, width, _height, height):
        self.width = int(width)
        self.height = int(height)

    def handle_window_DESTROY(self):
        self.alive = False

    def handle_window_TITLE(self, _str, *title):
        self.title = " ".join(title)

    def handle_window_GET_DIMENSIONS(self, _width, width, _height, height):
        self.width = width
        self.height = height

    def handle_window_NEW_CONTENT(self):
        pass

    def handle_window_NEW_ICON(self):
        pass

    def handle_window_START_THROBBER(self):
        self.throbbing = True

    def handle_window_STOP_THROBBER(self):
        self.throbbing = False

    def handle_window_SET_SCROLL(self, _x, x, _y, y):
        self.scrollx = int(x)
        self.scrolly = int(y)

    def handle_window_UPDATE_BOX(self, _x, x, _y, y, _width, width, _height, height):
        # pylint: disable=locally-disabled, no-self-use

        x = int(x)
        y = int(y)
        width = int(width)
        height = int(height)

    def handle_window_UPDATE_EXTENT(self, _width, width, _height, height):
        self.content_width = int(width)
        self.content_height = int(height)

    def handle_window_SET_STATUS(self, _str, *status):
        self.status = (" ".join(status))

    def handle_window_SET_POINTER(self, _ptr, ptr):
        self.pointer = ptr

    def handle_window_SET_SCALE(self, _scale, scale):
        self.scale = float(scale)

    def handle_window_SET_URL(self, _url, url):
        self.url = url

    def handle_window_GET_SCROLL(self, _x, x, _y, y):
        self.scrollx = int(x)
        self.scrolly = int(y)

    def handle_window_SCROLL_START(self):
        self.scrollx = 0
        self.scrolly = 0

    def handle_window_REDRAW(self, act):
        if act == "START":
            self.browser.current_draw_target = self
            self.plotted = []
            self.plotting = True
        else:
            self.browser.current_draw_target = None
            self.plotting = False

    def handle_window_CONSOLE_LOG(self, _src, src, folding, level, *msg):
        self.log_entries.append((src, folding == "FOLDABLE", level, " ".join(msg)))

    def handle_window_PAGE_STATUS(self, _status, status):
        self.page_info_state = status

    def load_page(self, url=None, referer=None):
        if url is not None:
            self.go(url, referer)
        self.wait_loaded()

    def wait_start_loading(self):
        while not self.throbbing:
            self.browser.farmer.loop(once=True)

    def wait_loaded(self):
        self.wait_start_loading()
        while self.throbbing:
            self.browser.farmer.loop(once=True)

    def handle_plot(self, *args):
        self.plotted.append(args)

    def redraw(self, coords=None):
        if coords is None:
            self.browser.farmer.tell_monkey("WINDOW REDRAW %s" % self.winid)
        else:
            self.browser.farmer.tell_monkey("WINDOW REDRAW %s %s" % (
                self.winid, (" ".join(coords))))
        while not self.plotting:
            self.browser.farmer.loop(once=True)
        while self.plotting:
            self.browser.farmer.loop(once=True)
        return self.plotted

    def clear_log(self):
        self.log_entries = []

    def log_contains(self, source=None, foldable=None, level=None, substr=None):
        if (source is None) and (foldable is None) and (level is None) and (substr is None):
            assert False, "Unable to run log_contains, no predicate given"

        for (source_, foldable_, level_, msg_) in self.log_entries:
            ok = True
            if (source is not None) and (source != source_):
                ok = False
            if (foldable is not None) and (foldable != foldable_):
                ok = False
            if (level is not None) and (level != level_):
                ok = False
            if (substr is not None) and (substr not in msg_):
                ok = False
            if ok:
                return True

        return False

    def wait_for_log(self, source=None, foldable=None, level=None, substr=None):
        while not self.log_contains(source=source, foldable=foldable, level=level, substr=substr):
            self.browser.farmer.loop(once=True)


def farmer_test():
    '''
    Simple farmer test
    '''

    browser = Browser(quiet=True)
    win = browser.new_window()

    fname = "test/js/inline-doc-write-simple.html"
    full_fname = os.path.join(os.getcwd(), fname)

    browser.pass_options("--enable_javascript=0")
    win.load_page("file://" + full_fname)

    print("Loaded, URL is {}".format(win.url))

    cmds = win.redraw()
    print("Received {} plot commands".format(len(cmds)))
    for cmd in cmds:
        if cmd[0] == "TEXT":
            text_x = cmd[2]
            text_y = cmd[4]
            rest = " ".join(cmd[6:])
            print("{} {} -> {}".format(text_x, text_y, rest))

    browser.pass_options("--enable_javascript=1")
    win.load_page("file://" + full_fname)

    print("Loaded, URL is {}".format(win.url))

    cmds = win.redraw()
    print("Received {} plot commands".format(len(cmds)))
    for cmd in cmds:
        if cmd[0] == "TEXT":
            text_x = cmd[2]
            text_y = cmd[4]
            rest = " ".join(cmd[6:])
            print("{} {} -> {}".format(text_x, text_y, rest))

    browser.quit_and_wait()

    class FooBarLogin(Browser):
        def handle_ready_login(self, lwin):
            lwin.send_username("foo")
            lwin.send_password("bar")
            lwin.go()

    fbbrowser = FooBarLogin(quiet=True)
    win = fbbrowser.new_window()
    win.load_page("https://httpbin.org/basic-auth/foo/bar")
    cmds = win.redraw()
    print("Received {} plot commands for auth test".format(len(cmds)))
    for cmd in cmds:
        if cmd[0] == "TEXT":
            text_x = cmd[2]
            text_y = cmd[4]
            rest = " ".join(cmd[6:])
            print("{} {} -> {}".format(text_x, text_y, rest))

    fname = "test/js/inserted-script.html"
    full_fname = os.path.join(os.getcwd(), fname)

    browser = Browser(quiet=True)
    browser.pass_options("--enable_javascript=1")
    win = browser.new_window()
    win.load_page("file://" + full_fname)
    print("Loaded, URL is {}".format(win.url))

    win.wait_for_log(substr="deferred")

    # print("Discussion was:")
    # for line in browser.farmer.discussion:
    #    print("{} {}".format(line[0], line[1]))


if __name__ == '__main__':
    farmer_test()
