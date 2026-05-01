# Packet Processing 1: My pings fly Spirit's middle seats

_This will be my first ever blog written as myself. I plan to write a series of four
blogs, each reimplementing a layer of the typical networking stack, starting from
Ethernet frames, and ending at HTTPS._

## Preface

There's a particular kind of restlessness that comes with a San Mateo summer noon — a cold Baridi Mule from Peet's, the Pedestrian Mall alive with tech bros joyfully walking out for lunch. As for me, I'm blessed with a rare pocket of free time in the gap between two internships, and it just feels criminal not to fill it with something.

<!-- TODO: Add picture from Peet's -->

At this point, one might spin up a Next.js project with `npx`, wire together a beautiful set of `Go` microservices orchestrated by `Kubernetes`, routed using `traefik`. Definitely not an invented story, since high school me definitely have done this. As long as it works.



At this point, one usually starts spinning up a Next.js project with `npx create-next-app@latest infinitely-scalable-microservice-app`, make a fun and convoluted web, and call it a day (I have friends who actually did this. No hate). Hold on for a moment though. Say your app sends out a packet. The app is in a Docker container. The docker networking stack take your packet out of the network namespace with a veth. The veth spits out your packet to the proxy server. The proxy server do some parsing and HTTP rewriting. The packet is then sent out of the namespace with the veth. The docker stack then send it to the kernel's stack. The kernel stack runs it through iptables, match it to the buffer. 

## Postscript

Of course, https://dl.acm.org/doi/pdf/10.1145/3419394.3423620
