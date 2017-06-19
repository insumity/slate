from gensim.models import *
model = word2vec.Word2Vec([['0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1']], size=1000, window=20, min_count=1, iter=100000)
model.predict_output_word(['0', '1', '0'])
