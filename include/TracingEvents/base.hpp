#ifndef BASE_EVENT_HPP
#define BASE_EVENT_HPP

#include <mutex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <cassert>
#include <queue>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <stack>
#include <algorithm>
#include <ctype.h>

#if defined(__APPLE__)
#include <xpc/xpc.h>
#include <mach/message.h>
#endif

#define LOGURU_WITH_STREAMS 1
#include "loguru.hpp"


typedef uint64_t tid_t;
typedef int event_type_t;

#if !defined(__APPLE__)
#include <cstring>
typedef uint64_t addr_t;
#endif


//EventType::event_type_t
#define MSG_EVENT                   1
#define MR_EVENT                    2
#define FAKED_WOKEN_EVENT           3
#define INTR_EVENT                  4
#define WQNEXT_EVENT                5
#define TSM_EVENT                   6
#define WAIT_EVENT                  7
#define DISP_ENQ_EVENT              8
#define DISP_DEQ_EVENT              9
#define DISP_INV_EVENT              10
#define TMCALL_CREATE_EVENT         11
#define TMCALL_CANCEL_EVENT         12
#define TMCALL_CALLOUT_EVENT        13
#define BACKTRACE_EVENT             14
#define SYSCALL_EVENT               15
#define VOUCHER_EVENT               16
#define VOUCHER_CONN_EVENT          17
#define VOUCHER_DEALLOC_EVENT       18
#define VOUCHER_TRANS_EVENT         19
#define CA_SET_EVENT                20
#define CA_DISPLAY_EVENT            21
#define BREAKPOINT_TRAP_EVENT       22
#define RL_OBSERVER_EVENT           23
#define EVENTREF_EVENT              24
#define NSAPPEVENT_EVENT            25
#define DISP_MIG_EVENT              26
#define RL_BOUNDARY_EVENT           27
#define APPLOG_EVENT                28

class EventCategory {
	typedef enum {
		Semantics,
		Structure
	} event_category_t;
	std::map<event_type_t, event_category_t> event_category_map;
public:
	EventCategory() {
//#define MSG_EVENT                   1
		event_category_map[MSG_EVENT] = Structure; 
//#define MR_EVENT                    2
		event_category_map[MR_EVENT] = Structure; 
//#define FAKED_WOKEN_EVENT           3
		event_category_map[FAKED_WOKEN_EVENT] = Structure; 
//#define INTR_EVENT                  4
//#define WQNEXT_EVENT                5
//#define TSM_EVENT                   6
//#define WAIT_EVENT                  7
		event_category_map[WAIT_EVENT] = Semantics; 
//#define DISP_ENQ_EVENT              8
		event_category_map[DISP_ENQ_EVENT] = Structure; 
//#define DISP_DEQ_EVENT              9
//#define DISP_INV_EVENT              10
		event_category_map[DISP_INV_EVENT] = Structure; 
//#define TMCALL_CREATE_EVENT         11
		event_category_map[TMCALL_CREATE_EVENT] = Structure; 
//#define TMCALL_CANCEL_EVENT         12
//#define TMCALL_CALLOUT_EVENT        13
		event_category_map[TMCALL_CALLOUT_EVENT] = Structure; 
//#define BACKTRACE_EVENT             14
		event_category_map[BACKTRACE_EVENT] = Semantics; 
//#define SYSCALL_EVENT               15
		event_category_map[SYSCALL_EVENT] = Semantics;
//#define VOUCHER_EVENT               16
//#define VOUCHER_CONN_EVENT          17
//#define VOUCHER_DEALLOC_EVENT       18
//#define VOUCHER_TRANS_EVENT         19
//#define CA_SET_EVENT                20
//#define CA_DISPLAY_EVENT            21
//#define BREAKPOINT_TRAP_EVENT       22
//#define RL_OBSERVER_EVENT           23
//#define EVENTREF_EVENT              24
//#define NSAPPEVENT_EVENT            25
//#define DISP_MIG_EVENT              26
//#define RL_BOUNDARY_EVENT           27
		event_category_map[RL_BOUNDARY_EVENT] = Structure;
//#define APPLOG_EVENT                28
		event_category_map[APPLOG_EVENT] = Semantics; 
	}

	bool is_semantics_event(event_type_t id) {
		if (event_category_map.find(id) == event_category_map.end())
			return false;
		return event_category_map[id] == Semantics;
	}

	bool is_structure_event(event_type_t id) {
		if (event_category_map.find(id) == event_category_map.end())
			return false;
		return event_category_map[id] == Structure;
	}

	bool is_tfl_filtered(event_type_t id) {
		return id == SYSCALL_EVENT || id == APPLOG_EVENT || id == BACKTRACE_EVENT;
	}
};
	
class EventBase;
class MsgHeader;
class MsgEvent;
class WaitEvent;
class IntrEvent;
class WorkQueueNextItemEvent;
class TimeshareMaintenanceEvent;
class MakeRunEvent;
class FakedWokenEvent;
class TimerCreateEvent;
class TimerCalloutEvent;
class TimerCancelEvent;
class SyscallEvent;
class BacktraceEvent;
class BlockEnqueueEvent;
class BlockDequeueEvent;
class BlockInvokeEvent;
class VoucherEvent;
class VoucherConnectEvent;
class VoucherTransitEvent;
class VoucherDeallocEvent;
class BankEvent;
class CASetEvent;
class CADisplayEvent;
class BreakpointTrapEvent;
class RunLoopObserverEvent;
class CoreGraphicsRefEvent;
class NSAppEventEvent;
class DispatchQueueMIGServiceEvent;
class RunLoopBoundaryEvent;
class AppLogEvent;

class ProcessInfo {
private:
    tid_t tid;
    pid_t pid;
    std::string procname;
public:
    ProcessInfo(tid_t tid_, std::string procname_) {tid = tid_; procname = procname_; pid = -1; valid_procname();}
    ProcessInfo(tid_t tid_, pid_t pid_, std::string procname_) {tid = tid_; procname = procname_; pid = pid_; valid_procname();}
    ProcessInfo(ProcessInfo &p) {tid = p.get_tid(); procname = p.get_procname(); pid = p.get_pid(); valid_procname();}

    tid_t get_tid(void) {return tid;}
    pid_t get_pid(void) {return pid;}
	void valid_procname() {
		bool valid = true;
		for (int i = 0; i < procname.size(); i++) {
			if (!(isalpha(procname[i]) \
                || isspace(procname[i]) \
                || isdigit(procname[i]) \
                || procname[i] == '_' \
                || procname[i] == '-')) {
				valid = false;
				break;
			}
		}
		if (!valid)
			procname="";
	}

    std::string& get_procname(void) {return procname;}

    void override_procname(std::string _procname) {procname = _procname;}
    void set_pid(pid_t _pid) {pid = _pid;}

    void update_process_info(tid_t tid_, pid_t pid_, std::string procname_) {
        if (pid_ > 0)
            pid = pid_;
        tid = tid_;
        if (procname_.size() > 0)
        procname = procname_;
    }

    void update_process_info(ProcessInfo &p) {
        if (pid == -1)
            pid = p.get_pid();
        else if (p.get_pid() == -1)
            p.set_pid(pid);
        if (procname.size() == 0)
            procname = p.get_procname();
        else if (procname.size() > 0)
            p.override_procname(procname);
    }
};

class TimeInfo {
    double begin_time;
    double finish_time;
public:
    TimeInfo(double begin_time_): begin_time(begin_time_), finish_time(0.0) {};
    void override_timestamp(double new_timestamp) {begin_time = new_timestamp;}
    void set_finish_time(double time) {finish_time = time;}
    double get_abstime(void) {return begin_time;}
    double get_finish_time(void) {return finish_time;}
    
};

class EventType {
public:
    typedef int event_type_t;
    event_type_t event_type;
private:
    std::string op;
public:
    EventType(event_type_t event_type_, std::string op_) {op = op_; event_type = event_type_;}
    event_type_t get_event_type() {return event_type;}
    std::string get_op(void) {return op;}
};

class EventBase: public ProcessInfo, public TimeInfo, public EventType {
    EventBase *event_peer;
    EventBase *event_prev;
    uint32_t core_id;
    uint64_t group_id;
    bool complete;
    uint64_t tfl_index;
	std::list<uint64_t> propagated_frames;

public:
    EventBase(double timestamp_, int event_type_, std::string op_, tid_t tid_, uint32_t core_id_, std::string procname_ = "");
    EventBase(EventBase *); //deep copy
    virtual ~EventBase() {}

    void set_event_peer(EventBase *peer_) {event_peer = peer_;}
    void set_event_prev(EventBase *prev_) {event_prev = prev_;}
    void set_group_id(uint64_t gid_) {group_id = gid_;}
    void set_complete(void) {complete = true;}
    void set_tfl_index(uint64_t i) {tfl_index = i;}
	void set_propagated_frame(std::list<uint64_t> frames) { propagated_frames = frames;}
	EventBase *get_prev_event(void) {return event_prev;}
    EventBase *get_event_peer(void) {return event_peer;}
    uint32_t get_coreid(void) {return core_id;}
    uint64_t get_group_id(void) {return group_id;}
    bool is_complete(void) {return complete;}
    uint64_t get_tfl_index() {return tfl_index;}
	std::string replace_blank(std::string);

    virtual void decode_event(bool is_verbose_, std::ofstream &outfile);
    virtual void streamout_event(std::ofstream &outfile);
    virtual void streamout_event(std::ostream &out);
    virtual void tfl_event(std::ofstream &outfile);
};
#endif
