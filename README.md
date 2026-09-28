[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![CI](https://github.com/ghex-org/oomph/actions/workflows/CI.yml/badge.svg)](https://github.com/ghex-org/oomph/actions/workflows/CI.yml)
[![Spack](https://github.com/ghex-org/oomph/actions/workflows/spack.yml/badge.svg)](https://github.com/ghex-org/oomph/actions/workflows/spack.yml)
# OOMPH

**Oomph** is a library for enabling high performance point-to-point, asynchronous communication over
different fabrics. It leverages the ubiquitous MPI library as well as UCX and Libfabric. Both
device and host memory are supported. A subset of functionality is also supported with NCCL. Under
the hood it uses [hwmalloc](https://github.com/ghex-org/hwmalloc) for memory registration.

**selling points**
- lightweight, fast
- ABI stable
- variety of backends
- can be used in threaded applications
- tagged asynchronous send/recv
- callback based interface

# Overview

Below a few important features of the library are highlighted and illustrated with code snippets.
Note, that this library
expects that at least an MPI implementation is available.

## Context
In order to use **oomph** a context object needs to be created.  The context object manages the
lifetime of the transport layer specific infrastructure. The context must therefore be created in a
serial part of the code.

```cpp
oomph::context ctxt{MPI_COMM_WORLD, true};

```
All of its member functions are thread-safe if the second argument of the constructor is true
(optional, default = true).  The first argument is the application's MPI communicator, which must be
kept alive until the context goes out of scope. The MPI communicator is duplicated within **oomph**
in order to protect against other calls to MPI in the user's code.

## Message Buffer

Messages must be sent through a message buffer which can be created from the context or the
communicator (see also below).
```cpp
oomph::message_buffer<int> msg = ctxt.make_buffer<int>(100);
```
Device memory can be requested using
```cpp
// allocate on device 0
oomph::message_buffer<int> msg = ctxt.make_device_buffer<int>(100, 0);
```
Note, that the underlying memory manager (hwmalloc) will always allocate on the host, and will
mirror memory on the host. This does not imply that communications will always go through the host,
however. GPU aware transport layer functionality is fully supported.

## Communicator

The communicator allows to schedule send and receive operations.  A communicator object is thread
compatible. Usually one communicator is created per thread, however, multiple instances per thread
are possible. One must not use another thread's communicator object.

```cpp
oomph::communicator comm = ctxt.get_communicator();
```

### Send/Recv

Send and receive operations return request objects which can be used to monitor completion of the
scheduled operation.
```cpp
// send a message to rank 1 with tag 42
oomph::send_request req = comm.send(msg, 1, 42);
// check whether communication has finished
if (req.is_ready()) { /* do something */ }
// check whether communication has finished and progress transport layer
if (req.test()) { /* do something */ }
// wait until communication has finished
req.wait();
```
The receive requests additionally expose a member funcion to cancel the scheduled operation if
possible.
```cpp
// receive a message from rank 1 with tag 42
oomph::recv_request req = comm.recv(msg, 1, 42);
// cancel receive request
req.cancel();
```

Every send and receive operation can be used in conjunction with a callback which is invoked once
the operation completes.
```cpp
oomph::message_buffer<int> msg = ctxt.make_buffer<int>(100);
oomph::send_request req = comm.send(msg, 1, 42,
    [](message_buffer<int> & msg, int rank, int tag)
    {
        // do something with the message
    });
// ...
req.wait();
```
Note, that the signature of the callback depends on the function it is used with. For example, the
communicator will take over a message's lifetime management when the message is passed by r-value
reference:
```cpp
oomph::message_buffer<int> msg = ctxt.make_buffer<int>(100);
oomph::send_request req = comm.send(std::move(msg), 1, 42,
    [](message_buffer<int> msg, int rank, int tag)
    {
        // do something with the message
    });
// At this point we can forget the message or assign some new message to it
msg = ctxt.make_buffer<int>(10);
// ...
req.wait();
```
Here, the callback will need to receive the message argument by value.

Note: recursive calls to the communicator from within a callback are explicitely allowed.

### Progress

Instead of checking the request objects, the underlying transport layer can also be progressed manually
which may be useful if one is working with callbacks:
```cpp
oomph::message_buffer<int> msg = ctxt.make_buffer<int>(100);
bool completed = false;
/* no need for request */ comm.send(msg, 1, 42,
    [&completed](message_buffer<int> & msg, int rank, int tag)
    {
        // do something with the message
        completed = true;
    });
// ...
// progress a number of times
comm.progress();
comm.progress();
// or progress until some event is triggered
while(!completed) { comm.progress(); }
```

### Groups

Communicators expose group functionality as provided by NCCL (with
ncclGroupStart and ncclGroupEnd). For non-NCCL backends the group functionality
is a no-op. For NCCL using the group functionality can be a both a requirement
to avoid deadlocks (communication within a group can make progress
independently, while outside of a group communication is ordered) and for
performance (a single device kernel is submitted for a NCCL group). 

Groups are created by explicitly starting and ending the group:

```cpp
comm.start_group();
oomph::send_request sreq = comm.send(smsg, 1, 0);
oomph::recv_request rreq = comm.recv(rmsg, 1, 0);
comm.end_group();

// With NCCL, no progress will be made until after the group ends
sreq.wait();
rreq.wait();
```

### Stream awareness

Some backend implementations can schedule communication on a GPU stream.
Currently only the NCCL backend makes use of this. All other backends ignore
the stream argument. To query if a backend is stream-aware use the
`is_stream_aware` member query on a communicator. The stream can be passed as
an optional last parameter to `send` or `recv`:

```cpp
if (comm.is_stream_aware()) {
    // Schedule communication on the default CUDA stream if the backend is
    // stream aware
    cudaStream_t stream = 0;
    oomph::send_request req = comm.send(msg, 1, 0, stream);
}
```

### NCCL restrictions

NCCL has significantly different semantics from MPI, libfabric, and UCX which
is reflected in a number of restrictions on how the NCCL communicator can be
used:

- Tags are not supported by NCCL and ignored by the backend. Communication
  order on different ranks must match (except within NCCL groups where there is
  some flexibility). This also means that e.g. recv should not be called before
  send unless within a NCCL group.
- The `thread_safe` option for the NCCL communicator is not supported because
  of the above ordering restrictions.
- Cancellation is not supported.
- `wait` and `progress` are disallowed when a NCCL group is active as no
  progress can be made until a NCCL group is ended and submitted.
- Send/recv to own rank is supported within NCCL groups. Outside of a group,
  self-send/recv will throw an exception because NCCL's order-based matching
  requires the atomic submission that groups provide.

The NCCL backend is primarily designed for use in GHEX where these differences
can be hidden from the user.

## Acknowledgments
This work was financially supported by the PRACE project funded in part by the EU's Horizon 2020
Research and Innovation programme (2014-2020) under grant agreement 823767.
