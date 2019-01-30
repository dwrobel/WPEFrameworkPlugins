#pragma once

#include "../Module.h"

namespace WPEFramework {
namespace TestCore {

class TestCommandDescription : public Core::JSON::Container {
    private:
        TestCommandDescription(const TestCommandDescription&) = delete;
        TestCommandDescription& operator=(const TestCommandDescription&) = delete;

    public:
        TestCommandDescription()
            : Core::JSON::Container()
            , Description()
        {
            Add(_T("description"), &Description);
        }

        ~TestCommandDescription() = default;

    public:
        Core::JSON::String Description;
    };

    class TestCommandName : public Core::JSON::Container {
    private:
        TestCommandName(const TestCommandName&) = delete;
        TestCommandName& operator=(const TestCommandName&) = delete;

    public:
        TestCommandName()
            : Core::JSON::Container()
            , Name()
        {
            Add(_T("name"), &Name);
        }

        ~TestCommandName() = default;

    public:
        Core::JSON::String Name;
    };

    class TestCommandSignature : public Core::JSON::Container {
    private:
        TestCommandSignature(const TestCommandSignature&) = delete;
        TestCommandSignature& operator=(const TestCommandSignature&) = delete;

    public:
        class Parameter : public Core::JSON::Container {
        public:
            Parameter()
                : Core::JSON::Container()
                , Name()
                , Type()
                , Comment()

            {
                AddFields();
            }

            Parameter(const string& name, const string& type, const string& comment)
                : Core::JSON::Container()
                , Name()
                , Type()
                , Comment()
            {
                // currently there is something wrong with the copy constructor of Core::JSON::String
                // for example: invoking ToString() will not generate proper string
                // that's why we want to copy elements using opertor=
                this->Name = name;
                this->Type = type;
                this->Comment = comment;

                AddFields();
            }

            Parameter(const Parameter& copy)
                : Core::JSON::Container()
                , Name()
                , Type()
                , Comment()
            {
                // currently there is something wrong with the copy constructor of Core::JSON::String
                // for example: invoking ToString() will not generate proper string
                // that's why we want to copy elements using opertor=
                this->Name = copy.Name;
                this->Type = copy.Type;
                this->Comment = copy.Comment;

                AddFields();
            }

            Parameter& operator=(const Parameter& rhs)
            {
                this->Name = rhs.Name;
                this->Type = rhs.Type;
                this->Comment = rhs.Comment;

                return *this;
            }

            ~Parameter() = default;

        private:
            void AddFields()
            {
                Add(_T("name"), &Name);
                Add(_T("type"), &Type);
                Add(_T("comment"), &Comment);
            }

        public:
            Core::JSON::String Name;
            Core::JSON::String Type;
            Core::JSON::String Comment;
        };

    public:
        TestCommandSignature()
            : Core::JSON::Container()
            , Input()
            , Output()
        {
            Add(_T("input"), &Input);
            Add(_T("output"), &Output);
        }

        ~TestCommandSignature() = default;

    public:
        Core::JSON::ArrayType<Parameter> Input;
        Parameter Output;
};
} // namespace TestCore
} // namespace WPEFramework
