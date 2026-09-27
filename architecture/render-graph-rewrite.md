# Render Graph Rewrite

## What problem am I solving?

The render graph I created for nitro helios was usable for a while but as i begone to use it i started seeing the problem behind my current implementation.
The first one was Write-after-Read and Write-after-Write. There was no way to provide an order to this sequences. I had to make use of tactics like a Producer and Extenders of a resource which is more like a patch than a fix of the underlying problem.

Another problem i ran into while working on Occlusion Culling was Temporal resources. This are resources that have a history based on N frames-in-flight. They have like a previous and current for a frame. There was to identify these resource more less about using them to build the dependency graph for the render passes.

So now I decided to separate some things from the render graph. The render graph is currently responsible for managing physical resources, compiling the graph dealing with frame resize and so on. I plan on separating these responsibility to their individual paths.

My current goal is to think of a way to separate a resource manager from a graph builder, what resources do i need, and how do i separate a physical resource from a logical resource.

## What is a logical resource?

A logical resource is one who points to a specific physical resource. It is used to fetch the physical resource from the resource manager.

## What is a physical resource?

A physical resource is the actual resource (texture or buffer) that will be used during the render execution. From what I've seen there needs to be about 4 kinds of physical resource.

1. Persistent: These ones will be reused for each frame and can not be memory-aliased with any other resource.
2. Transient: These can be memory-aliased with a similar resource that has the same size and requirement.
3. Temporal/History: These needs to be made of N resource slot. I plan on keeping a frame idx on the resource manager. I know with this that i can rotate and fetch the current and previous resource for a frame.
4. Per-Frame resource: these resource are not temporal. They are resource which are switched per frame. I've come to realize that I need to separate them as well.

## When does a logical resource become physical?

A logical resource in it self does not become physical. THe physical resource is built by the resource manager. The logical resource in it self is used by the render graph to build the dependency for the render passes and perform a topological sort with barrier insertion.
All fetching of resources comes from the resource manager. An optimization that comes in mind is to generate a hash of the passes and have a way to compute this and send to the consumer to hold. If it has not changed we make use of the cached compiled frame graph. if not we can re compile and save the frame graph and the hash value as well

## What does a resource version represent?

A resource version is meant for a logical resource. I make use of these so that I can keep track of different versions of a resource so that i don't run into situations like Write-After-Read or Write-After-Write. When a resource is added to a write block the version is incremented. I use this so that the next pass to be registered that is either reading or writing to that particular resource knows that the version their using is this one. This was my actual plan to solve WAR and WAW.

## What does a pass represent?

A pass in itself is suppose to be just something that does read/write on specific resources and as a callback function that is executed to perform a render pass or a compute pass.

## What does compilation produce?

Well a compilation is actually meant to produce a ordered FrameGraph which include passes and resource barrier transitions.
These will be used by the render graph to execute the passes in a topological sorted order.
My plan for this step will be to produce the FrameGraph and execute that Graph.

## Core Graph Model

Previously I made use of Resources to build the dependency between passes.
Resource -> Pass
This made me to later on introduces some weird Producer/Extender semantic for Resource Access in the pass descriptor.

Now I've decided to make use of resource versions to build the implicit dependency
ResourceVersion -> Pass -> ResourceVersion

E.g
X1 -> Pass A -> X2 -> Pass B -> X3

This way each pass can access different logical resource versions and each version can create a dependency between it's producer pass and it's previous version producer pass.
A resource version has exactly one producer pass.
A resource version may have zero or many consumer passes.

## Resource Types

For now I have two main resource types

1. Textures
2. Buffers

Textures need this info to be created

1. Size (width and height)
2. Format (ColorRGBA8, Depth32, Stencil etc)
3. Usage (Storage, Render Target etc)
4. Mip map level count
5. Type (Flat or Cube)

Buffers need this to be create

1. Size (in bytes)
2. Usage (Vertex,Indirect etc)
3. Storage Mode (Shared, GPU, Dynamic)
4. Initial Data (nullable);

Sometimes these resource sizes are relative to the viewport and they can be scaled up and down based on this. Some are just absolute values so they can be created from that.

Now in terms of storage, there are 4 types:

1. Persistent: THese are resources that will be reused for each frame. These resources can not be aliased with any other resource.
2. Transient: These resource can be aliased (share memory/heap) with other resource but there's a condition. It must be at less than or equal to the memory block usage and the resource can not be accessed at the same point.
3. Temporal: These resources are more like Persistent resource but the one thing that differentiate them is the history with a current and previous semantic. These have N resources, There's a current and previous resource.
4. Per-Frame: These at first sound like temporal but there's a difference. There's no history and current/previous semantic. These are N resources that are created and used per-frame. N in this case is the Max Frames in flight (currently 2 for me). It's like a ring buffer with 2 resources based on the current frame idx.
   Per-Frame and Temporal can share the same allocation mechanism but with different ways to access it.

## Entities to implement

These are the things that i want to work on before i move on to the actual RenderGraphBuilder

1. Resource + Resource Declaration
2. Resource Allocation (How to store and access each resource type)
3. Creating Resources (Persistent, Temporal, Transient, Per-Frame).
4. Introducing previous/current semantic for temporal
5. Resource Sizing
6. DeferredResource Destruction (Optional)
7. Resource State Tracking
8. Creating/Deleting Transient Resource (Especially Screen Relative ones tied with Absolute Resources)

## Pending

1. Logical Resource
2. Resource Versions
3. Logical to Physical lookup
4. Pass Declaration (or PassBuilder I'll decide that later)
5. Dependency Graph Generation
6. Resource State Transition (Barriers) Generation
7. Compiled Frame Graph

## Physical -> Logical Resource

Buffer & Texture -> Logical Resources Type
A resource can be Transient or Persistent, it can also be Per-Frame or Single. A Logical resource does not care about this.
It just needs to know the resource type. Where it get's tricky is with temporal. Cause now we're not dealing with one instance of the resource for that frame. We have two, which are current and previous.
An idea that comes to mind is to have the current and previous as resource versions that can be used but the issue here with that is that during the barrier generation, I'll need to distinguish between previous and current for the transition.
For that fix i think having a TextureHistoryTransition/BufferHistoryTransition would help in this case to know which would be previous while the normal
TextureTransition/BufferTransition would be for the current implementation.

So there are two types of LogicalResource

1. LogicalResource (will need to change this name)
2. TemporalLogicalResource (This will be made of two logicalResource (current and previous))

## Resource Version

A Logical Resource can have multiple resource versions. Resource versions help to solve problems Like WAR (Write After Read) and WAW (Write After Write).
The Resource version is what will be used to to build the Render Pass Dependency Graph Edges.
There are two problems that needs to be solved first:

1. How Resource Versions are stored and link to previous versions?
2. How a Logical Resource Version is incremented without letting the user handle this?

For the first question, My solution to that relies on splitting the two things, Logical Resource and Resource Version. The entire point of the render graph is to built the ordered list of render passes based on their reads/writes. So My goal is to keep the Resource Versions inside of the Logical Resource.
Each version can have a list of their reads/writes.
The Render Pass just needs to know the Logical Resource it consumes. This will be used for the resource transition later on.

This helps as well cause the flow for the dependency generation would be:

1. Add Render Pass idx as nodes
2. Loop through the resource versions.
3. Build the edges based on the reads and write
4. If there's a previous version, make it dependent on the previous version writes and reads
5. Just make sure that each resource version has a write

The api user won't get the logical resource object, rather the Handle to that logical resource, this way it solves the second problem, the versions could be incremented( appended to the list) without the user worrying about it.

The downside I can think of this is the render pass has no access to know what it reads or produces, in case I where to add a render pass context during execution. At the moment I can't see the usefulness for that yet has I plan to just pass the CommandBuffer and the ResourceManager during the execution of the RenderPass. Along with other required things.
