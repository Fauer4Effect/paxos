# paxos

## State Machine Replication

### Author: Kyle Fauerbach

This is an implementation of the Paxos state machine replication protocol as originally described by Lamport in *The Part-Time Parliament*. This specific implementation follows the design decisions made Kirsch and Amir in *Paxos for Systems Builders*. This implemenation is written in C and utilizes UDP sockets for communication. The current status of this project is that it is able to function in the normal case of operation. It goes through the Leader Election and Prepare Phases, can send a Proposal and confirm Accepts from the quorom.

Limitations:
  * There is no discovery mechanism. Peers are assumed to be known at start.
  * Reconcilliation is not implemented.
  * Does not write current state to disk.
  * Who starts the leader election. There is currently no controls on who starts the leader election. All servers will try at the same time and this could cause some churn in the system.

There is some instrumentation built in to the protocol for a demonstration that the protocol is able to select a leader and apply a client update. But there is not much in terms of an interface for another service to interact with the protocol.

---

For general use, code can be run as:

`./paxos -p <port> -h <host file>`

To implement the test instrumentation, you can use the `-t` flag. This will cause server 1 to generate a client update after the leader election has been completed. This allows you to see how the Proposal and Accepts are sent.

At startup, server 1 is designated to start the leader election. This is to ensure that we arrive at a stable state quickly. There currently is nothing about who should initiate leader election (i.e. sending the view change) for later views.

