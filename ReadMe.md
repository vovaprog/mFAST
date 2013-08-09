Introduction
============

mFast is an open source C++ implementation of FAST encoder. The FAST protocol (FIX Adapted for STreaming) is a technology standard developed
by FIX Protocol Ltd., specifically aimed at optimizing data representation on the network. It is used to support high-throughput, low latency
data communications between financial institutions.

mFAST was designed from the ground to provide an easy to use and yet efficient FAST encoder. Based on my initial experience to
QuickFAST[www.quickfast.org], I saw there were some room for improvement. However,  the
improvement couldn't be achieved without an architectural redesign. Therefore, I took it as a personal project and
worked on it mostly at night and weekends. After two and a half months of development, I am proudly to present this work.

Compared to QuickFAST, mFAST offers the following features:

An easier to use application type system
-----------------------------------------
The FAST encoding is performed with respect to a control structure called a template. A template
controls the encoding of a portion of the stream by specifying the order and structure of fields, their field operators and
the binary encoding representations to use. A FAST template is defined with a concrete syntax using XML format. One of the
most distinctive advance of mFAST over QuickFast is its ability to generate C++ application types using the XML specification.
If you are familiar with CORBA or DDS, this process is tantamount to the proxy/stub code generation using IDL compiler.

The following code snippet shows how to use the generated types:

    // the encoder setup steps are omitted
    const char* first = buf;
    const char* last = buf + buf_len;
    mfast::message_cref message = encoder.decode(first, last);
    if (message.id() == MarketData_cref::the_id)
    {
        MarketData_cref data = static_cast<MarketData_cref>(message.cref());
        std::string versionID = data.get_ApplVerID();
        uint32_t seq_no = data.get_MsgSeqNum().value();

        // C++11 syntax, not required for usage.
        for (auto element : data.get_MDEntries()) {  // MDEntries is a sequence
           uint32_t update_action =  element.get_MDUpdateAction();
           //...
        }
    }

Not only can the generated types  be read in a structured way, mFAST also provide an alternative interface using visitor pattern
to access a message in a generic way.

    using namespace mfast;

    // a generic message accessor
    struct message_printer // notice that we don't inherit from any class
    {
        indenter_t indenter_;

        template <typename PrimitiveTypes>
        void visit(const PrimitiveTypes& ref)
        { // for int32_cref, uint32_cref, int64_cref, uint64_cref, decimal_cref
          //     ascii_string_cref, unicode_string_cref, byte_vector_cref

            std::cout << indenter_ << ref.name() << ":" << ref << "\n";
        }

        bool pre_visit(const group_cref& ref)
        {
          std::cout << indenter_++ << ref.name() << ":\n";
          return true; // return true if we want its children to be visited
        }

        void post_visit(const group_cref& ref)
        {
          --indenter_;
        }

        bool pre_visit(const sequence_cref& ref)
        {
          std::cout << indenter_++ << ref.name() << ":\n";
          return true; // return true if we want its children to be visited
        }

        void post_visit(const sequence_cref& ref)
        {
          --indenter_;
        }

        bool pre_visit(std::size_t index, const sequence_element_cref& ref)
        {
          std::cout << indenter_++ <<  "[" << index << "]:\n";
          return true; // return true if we want its children to be visited
        }

        void post_visit(std::size_t index, const sequence_element_cref& ref)
        {
          --indenter_;
        }

        bool pre_visit(const message_cref& ref)
        {
          std::cout  << indenter_++ << ref.name() << "\n";
          return true; // return true if we want its children to be visisted
        }

        void post_visit(const message_cref&)
        {
          --indenter_;
        }
    };

    //////////////////////////////////////////
    // This is how the above visitor is used
    //////////////////////////////////////////
    message_cref message = encoder.encoder(stream);
    message_printer printer;
    message.accept_accessor(printer);

Lower external dependencies
---------------------------------------
Beside C++ standard library, the core mFAST encoder uses header only boost library to provide platform
portability. In addition, it uses TinyXML2 for XML parsing. However, TinyXML2 has only one source and one header file and it is
statically linked to mFAST applications.

Smaller memory footprint
-----------------------------------
The core of mFAST is an unique and very compact application type system. The encoder is designed as a visitor for writing values
to an instance of application type. This design makes the system very modular in terms of functionality. Even though mFAST does
not have the encoder yet. It would be implemented as another visitor for the message types and won't be linked to the executable unless
used.

To see how small mFAST application is compared to that of QuickFAST. I implemented two test programs for decode a FAST data stream
in a file using mFAST. One of them
(named fixed_template_test) used  mFAST type generator to produces application types on a sample FAST specification and then used
the generated type to drive the encoder. The second test program (generic_template_test) directly parsed a FAST specification and
used the parsed information to drive the encoder without the extra code generation step.

On the QuickFAST side, I used the PerformanceTest located in the QuickFAST example directory for comparison.
Like mFAST generic_template_test, the QuickFAST PerformanceTest program parsed a FAST specification on the fly and decoded an input data
stream from a file. However, it did not assemble the decoded messages. Instead, it notified the application via a callback object. Whenever
there's a new data field is processed, the QuickFAST encoder invoked the callback object to provide the name of the data field and the decoded value.

QuickFAST also have a GenericMessageBuilder class which actually build an entire message. In order to provide a functional equivalent program
to our mFAST counterparts (because both fixed_template_test and generic_template_test actually build entire messages), I modified the original PerformanceTest
to use the GenericMessageBuilder (coupled  with a NullMessageConsumer) and named the modified program as GenericBuilderPerformace.

To make fair comparison, both mFAST and QuickFAST libraries are statically linked into their respective applications. However,
QuickFAST depended Xerces and some other boost run-time libraries and those library were still dynamically linked into the executables.
On the other hand, the mFAST executables did not have those dependencies.

The following  are the sizes of executables compiled in MAC OS/X 10.8 with i686-apple-darwin11-llvm-g++-4.2 compiler using -O3 flag without debug information.

* mFAST fixed_template_test : 463,296 bytes
* mFAST generic_template_test : 621,076 bytes
* QuickFAST PerformanceTest : 1,317,148 bytes
* QuickFAST GenericBuilderPerformace : 1,450,236 bytes

As you can see, the mFAST executables are about 30% to 40% of the QuickFAST versions; which is quite a reduction, not to say that mFAST versions do not link to other
dynamic libraries as their QuickFAST counterparts.

Better runtime efficiency
----------------------------
To understand mFAST runtime efficiency, I used the above test programs to decode a data stream file with 30000 messages.
The following are the measured performance on my MacBook Pro 2009.

* mFAST fixed_template_test : 142 msec
* mFAST generic_template_test : 133 msec
* QuickFAST PerformanceTest : 148 msec
* QuickFAST GenericBuilderPerformace : 941 msec

Considering that QuickFAST PerformanceTest did not actually assemble the entire message,
it was still slightly slower than mFAST versions with messages being assembled.
In the case where messages were actually assembled, QuickFAST was about 7 times slower
than mFAST.


Conclusion
=============
Even though mFAST is only a encoder at current stage, it already provides better usability, smaller
memory footprint and superior runtime efficiency than QuickFAST. However there are still some
area that needs to be done :

1. Testing on platforms other than Mac OS/X.
2. Better documentation.
3. Of course, a mFAST encoder.
