using System;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace JsonModel
{
    public class RootModel
    {
        [JsonProperty("quiz")]
        public class quiz { get; set; }

    public class RootModel_quiz
    {
        [JsonProperty("maths")]
        public class maths { get; set; }

    public class RootModel_quiz_maths
    {
        [JsonProperty("q1")]
        public class q1 { get; set; }

    public class RootModel_quiz_maths_q1
    {
        [JsonProperty("answer")]
        public string answer { get; set; }

        [JsonProperty("options")]
        public List<string> options { get; set; }

        [JsonProperty("question")]
        public string question { get; set; }

    }

        [JsonProperty("q2")]
        public class q2 { get; set; }

    public class RootModel_quiz_maths_q2
    {
        [JsonProperty("answer")]
        public string answer { get; set; }

        [JsonProperty("options")]
        public List<string> options { get; set; }

        [JsonProperty("question")]
        public string question { get; set; }

    }

    }

        [JsonProperty("sport")]
        public class sport { get; set; }

    public class RootModel_quiz_sport
    {
        [JsonProperty("q1")]
        public class q1 { get; set; }

    public class RootModel_quiz_sport_q1
    {
        [JsonProperty("answer")]
        public string answer { get; set; }

        [JsonProperty("options")]
        public List<string> options { get; set; }

        [JsonProperty("question")]
        public string question { get; set; }

    }

    }

    }

    }

